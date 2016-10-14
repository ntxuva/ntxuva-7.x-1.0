<?php
// $id$

/**
 * @file
 * Implementation of Plivo-related functionality
 */

/**
 * Internal functions
 */

/*
 * Process incoming requests and responses from Plivo
 *
 * @param string $server set to name of the server (plivoframework or plivocore)
 *
 * @param string $plivo_action set to the action to be executed.
 *   It can assume any of the following values:
 *   - process_conference, to handle responses to conference call requests
 *   - process_inbound_calls, to handle incoming calls
 *   - process_inbound_text, to handle incoming text
 *   - process_outbound_calls, to handle the response to outgoing call requests
 *   - process_dial, to handle the response to the "dial" script command
 *   - process_get_input, to handle responses to "get input" script commands
 *   - process_hangup, to handle call termination
 *   - process_record and process_empty_recording, to handle
 *     responses to 'record' script commands
 *   - process_wait, to continue after wait command
 *   - process_sms_callback, to handle final responses to 'send sms'.
 *
 * @param string $cid with the cid of the call
 *
 * @return void
 */
function voipplivocore_call_handler($server, $plivo_action, $cid = NULL) {
  if (voip_debug()) {
    $msg = "in voipplivocore_call_handler() with plivo_action: $plivo_action, cid: $cid, and _REQUEST: " . print_r($_REQUEST, TRUE);
    $vars = array();
    watchdog('voipplivocore', $msg, $vars, WATCHDOG_INFO);
  }
  // validate $_REQUEST to avoid hack attempts
  if(!call_user_func('_voip'.$server.'_validate_request')) {
    $msg = '_voip_plivocore_call_handler called with invalid AccountSid: @sid';
    $params = array('@sid' => $incoming_sid);
    watchdog('voipplivocore', $msg, $params, WATCHDOG_WARNING);
    return FALSE; // TODO: should we return an error response to the server?
  }

  //Removing text options:
  if ($plivo_action == 'process_inbound_text'
    || $plivo_action == 'process_sms_framework') {
    if (isset($_REQUEST['Channel'])) {
      $channel = $_REQUEST['Channel'];
    }
    else {
      $channel = 'text';
    }

    $network = 'sms';
  }
  else {
    $channel = 'voice';
    $network = 'voip';
  }
  $from = $_REQUEST['From'];
  $to = $_REQUEST['To'];

  if($server == 'plivocloud') {
    //Normalize numbers
    $from = VoipPlivoCloudServer::NormalizeNumber($from);
    $to = VoipPlivoCloudServer::NormalizeNumber($to);
  }

  //At hangup the response returns only From value, making it impossible to track session, so making this hack to prevent creating new sessions.
  /*if($_REQUEST['HangupCause'] == 'NORMAL_CLEARING') {
    return;
  }*/
  // 'process_hangup' for incoming calls does not come with a $cid
  if (($plivo_action == 'process_hangup') && (!$cid)) {
    $call_id = $_REQUEST['CallUUID'];
    $cid = VoipCall::getCidFromCallId($call_id);
  }

  if ($cid) {
    $voip_session = VoipSession::getSessionByCid($cid);
  }
  else {
    $voip_session = VoipSession::getSessionByNumber($from, $to);
    if ($voip_session) {
      $cid = $voip_session->getCid();
    }
  }
  if (!$voip_session) {
    $voip_session = new VoipSession();
    $voip_session->setOriginNumber($from);
    $voip_session->setDestinationNumber($to);
  }

  // load the already existing call object, if any
  if ($cid) {
    $call = VoipCall::load($cid);
  }
  else {
    $call = new VoipCall();
  }

  // if the call is already hang up, ignore everything
  // Note: This happens whenever a call is hang up in the middle of a recording.
  // In that case, Plivo sends a notification both as part of the record
  // response as well as via a hangup request
  $is_hangup = $call->isHangup();
  if($is_hangup){
    if (voip_debug()) {
      $msg = "Call with cid: $cid is already hang up.";
      $vars = array();
      watchdog('voipplivocore', $msg, $vars, WATCHDOG_INFO);
    }
    return;
  }

  // handle incoming call and text requests
  if ($plivo_action == 'process_inbound_calls') {
    $call_sid = $_REQUEST['CallUUID']; // unique identifier of the call, 34 char CallUUID
    $call->setCallId($call_sid);
    $call->setSource($server);
    $dir_tmp = ($_REQUEST['Direction'] == 'inbound')
      ? VoipCall::INBOUND
      : VoipCall::OUTBOUND_DIAL;
    $call->setDirection($dir_tmp);
    $call->setAnsweredBy(VoipCall::ANSWERED_DRUPAL);
    $call->setCallStatus(VoipCall::RINGING);
    $call->setStartTime(REQUEST_TIME);
    $call->setCallerNumber($from);
    $call->setDestNumber($to);
    $call->setCallChannel($channel);
    $call->setCallNetwork($network);
  }
  else if ($plivo_action == 'process_inbound_text'
    || $plivo_action == 'process_sms_framework') {
    $call_sid = $_REQUEST['MessageUUID']; // unique identifier of the text, 34 char
    $call->setCallId($call_sid);
    $call->setSource($server);
    $dir_tmp = VoipCall::INBOUND;
    $call->setDirection($dir_tmp);
    $call->setAnsweredBy(VoipCall::ANSWERED_DRUPAL);
    $call->setCallStatus(VoipCall::IN_PROGRESS);
    $call->setStartTime(REQUEST_TIME);

    $call->setCallerNumber($from);
    $call->setDestNumber($to);
    $call->setCallChannel($channel);
    $call->setCallNetwork($network);
    if($plivo_action == 'process_sms_framework') {
      $call->setCallOrigin('smsframework');
    }
    $call->save();
  }
  else {
    // handle ongoing calls
    // handle first response to an outbound call
    if ($plivo_action == 'process_outbound_calls') {
      // store plivo call id for all outbound calls
      $call_id = $_REQUEST['CallUUID'];
      $call->setCallId($call_id);
      //Machine detection (works only with Plivo Cloud)
      if(isset($_REQUEST['Machine'])) {
        if($_REQUEST['Machine'] == 'true') {
          $tmp = VoipCall::ANSWERED_MACHINE;
        }
        else {
          $tmp = VoipCall::ANSWERED_HUMAN;
        }
      }
      else {
        $tmp = VoipCall::ANSWERED_HUMAN;
      }
      $call->setAnsweredBy($tmp);
    }
  }

  // update common call variables
  if (!$call->getCallChannel()) {
    $call->setCallChannel($channel);
  }
  if (!$call->getCallNetwork()) {
    $call->setCallNetwork($network);
  }
  if (!$call->getCallerNumber()) {
    $call->setCallerNumber($from);
  }
  if (!$call->getCallerName()) {
    $call->setCallerName($_REQUEST['CallerName']);
  }
  if (!$call->getDestNumber()) {
    $call->setDestNumber($to);
  }

  // get the cid of the call
  $cid = $call->getCid();
  if ($voip_session && !$voip_session->getCid()) {
    $voip_session->setCid($cid);
    $voip_session->save();
  }

  $plivo_call_status = $_REQUEST['CallStatus'];
  if (empty($plivo_call_status)) {
    $plivo_call_status = isset($_REQUEST['Status']) ? $_REQUEST['Status'] : 'in-progress';
  }

  // set start time for the call
  if ($plivo_call_status == 'in-progress') {
    $start_time = $call->getStartTime();
    if (!$start_time) {
      // start counting the call duration
      $call->setStartTime();
    }
  }

  // update call status
  // NOTE: sms callbacks do not have a call status associated with them
  // NOTE: when the user hangs up during a recording, Plivo sends the record
  // response with the recording data AND CallStatus set to 'completed'. Once that is done,
  // Plivo calls our process_hangup url to inform, once again, that the call has been hanged.

  if ($plivo_action == 'process_sms_callback'
    || $plivo_action == 'process_inbound_text'
    || $plivo_action == 'process_sms_framework'
    || $plivo_action == 'process_record'
    || $plivo_action == 'process_empty_recording'
    || $plivo_action == 'process_wait'
  ) {
    $call->SetCallStatus(VoipCall::IN_PROGRESS);
  }
  else {
    $call = voipplivocore_update_call_status($call, $plivo_call_status);
  }

  // save the call and initialize its script before setting internal variables
  $call->save();

  // set text channel variables
  if (strtolower($channel) == 'text') {
    $call->setTextVariables($_REQUEST['Text'], $from, $to, $network);
  }

  $is_hangup = $call->isHangup();

  // process responses to requests previously-issued by the call script
  if (!$is_hangup) {
    switch ($plivo_action) {
      default:
        $msg = 'Invalid plivo_action: @action';
        $params = array('action' => $plivo_action);
        $call->report_error('voipplivocore', $msg, $params);
        return FALSE;
        break;
      case 'process_inbound_calls':
      case 'process_outbound_calls':
      case 'process_hangup':
      case 'process_inbound_text':
      case 'process_sms_framework':
      case 'process_wait':
        // nothing special to be done here. the above commands have already
        // been processed in the previous lines
        break;
      case 'process_conference':
        // NOTE: in Plivo, conference calls are executed seperately
        //@todo: store some of returned information about conference? https://www.plivo.com/docs/xml/conference/#confcallbackurlparam
        break;
      case 'process_dial':
        $call = _voipplivocore_process_dial_response($server, $call, $_REQUEST);
        break;
      case 'process_get_input':
        $script = $call->getScript();
        $input = ($_REQUEST['Digits'] == 'no_input')
          ? '' : $_REQUEST['Digits'];
        $script->setVar('input_digits', $input);
        $call->setScript($script);
        break;
      case 'process_record':
        $script = $call->getScript();
        if($server == 'plivoframework') {
          $plivo_record_url = $_REQUEST['RecordFile'];
          $duration = $_REQUEST['RecordingDuration'];
          $digits = $_REQUEST['Digits'];
          //only local path supported in Plivo Framework however we need to transfer in Drupal files dir

          //We set this to same as recording_path because public url is not available in Plivo
          $file = _voipplivocore_file_save_upload($plivo_record_url);
          $script->setVar('recording_public_url', file_create_url($file->uri));
          $script->setVar('recording_fid', $file->fid);
          $script->setVar('recording_public_url', $file_url);
        }
        else {
          //Plivo Cloud
          if ($_REQUEST['RecordingDuration'] == -1) {
            //we must set hangup before _voipplivocore_transfer_file() as that
            //function takes long time, therfore resulting in process_hangup
            //command from Plivo to be executed before this call is set to hangup
            /*$call->SetCallStatus(VoipCall::COMPLETED);
            $call->setHangupReason(VoipCall::HANGUP_BY_USER);
            $call->save();*/
            voipplivocore_update_call_status($call, 'completed');
          }

          if($_REQUEST['status'] == 'completed') {
            voipplivocore_update_call_status($call, 'completed');
          }
          $plivo_record_url = $_REQUEST['RecordUrl'];

          $script->setVar('recording_public_url', $plivo_record_url);

          if ($_REQUEST['RecordingDuration'] > 0) {
            $duration = $_REQUEST['RecordingDuration'];
            $digits = $_REQUEST['Digits'];
          }
          else {
            //Could be calculated in voipscript_record_callback
            $duration = -1;
          }
        }
        //Common variables
        $script->setVar('recording_duration', $duration);
        $script->setVar('recording_digits', $digits);
        $call->setScript($script);

        break;
      case 'process_empty_recording':
        $script = $call->getScript();
        $script->setVar('recording_path', '');
        $script->setVar('recording_public_url', '');
        $script->setVar('recording_duration', 0);
        $script->setVar('recording_digits', NULL);
        $script->setVar('recording_fid', NULL);
        $call->setScript($script);
        break;

       case 'process_sms_callback':
         $call = _voipplivocore_process_sms_response($call, $_REQUEST);
         if(strtolower($call->getCallChannel()) == 'text') {
           // update sms status
           $call->save();
           // since 'process_sms_callback' is asynchronous, go back
           if($plivo_action == 'process_sms_callback') {
             return;
           }
         }
      break;
    }
  }

  if ($is_hangup) {
    // determine call duration
    $end_time = $call->setEndTime();
    $start_time = $call->getStartTime();

    if ($start_time) {
      $duration = $end_time - $start_time;
    }
    else {
      if($server == 'plivocloud') {
        $duration = isset($_REQUEST['Duration']) ? $_REQUEST['Duration'] : NULL;
      }
      else {
        //Not the default param, but you can set in plivo config :EXTRA_FS_VARS = variable_duration
        $duration = isset($_REQUEST['variable_duration']) ? $_REQUEST['variable_duration'] : NULL;
      }
    }
    $call->setDuration($duration);
  }

  // update the voipcall with the new values
  $call = $call->save();

  // update the call session object
  $cid = $call->getCid();
  if (!$voip_session->getCid()) {
    $voip_session->setCid($cid);
  }
  $voip_session->resetTimer();
  $voip_session->save();

  // get ready to process the call script

  // load global $user based on call user
  global $user;
  $stored_user = $user;
  $call->loadGlobalUser();
  if (voip_debug()) {
    watchdog('voipplivocore', 'Logged user %uid in.', array('%uid' => $user->uid));
  }
  if ($plivo_action == 'process_inbound_text') {
    $script = $call->getScript();
    $text_handler = $script->getVar('text_handler');
    $text_handler = !empty($text_handler) ? $text_handler : variable_get('voipcall_default_text_handler', '');
    if ($text_handler) {
      $continue_processing = call_user_func_array($text_handler,
        array(&$call));
      if(!$continue_processing) {
        return;
      }
    }
    else if(!(strtolower($call->getCallChannel()) == 'text')) {
      return;
    }
  }
  // process the call script
  voipplivocore_script_handler($server, $call);

  // If global user has been replaced for call user.
  // Process shouldn't go further, so just for good practice.
  if (!empty($stored_user)) {
    $user = $stored_user;
    if (voip_debug()) {
      watchdog('voipplivocore', 'Reverted to logged in user %uid', array('%uid' => $user->uid));
    }
  }

  return; // NOTE: this function MUST NOT return any value.  If it does, the
  //       output to Plivo will get messed up with Drupal hmtl...
}

/**
 * Process voipcall scripts
 *
 * @param $call, VoipCall instance with the call information
 *
 * @return boolean.  TRUE on success or FALSE on failure.  Check voip_error() for processing failure.
 */
function voipplivocore_script_handler($server, $call) {
  if (voip_debug()) {
    $msg = 'Entering script_handler with voipcall: ' . print_r($call, TRUE);
    watchdog('voipplivocore', $msg, array(), WATCHDOG_INFO);
  }
  $rc = TRUE;

  // get the script object associated with the call
  $script = $call->getScript();

  // check if call is in a hangup state
  $is_hangup = $call->isHangup();
  if ($is_hangup) {
    voipcall_process_hangup($call);
    return TRUE;
  }

  // process script commands
  $processing = TRUE;
  $response = '';
  while ($processing) {
    $cmd = $script->getNextCommand();
    if (!$cmd) {
      $processing = FALSE;

      // no more commands to be processed
      $stack_level = $script->getStackLevel();
      if ($stack_level > 0) {
        //return to previous stack
        $script->runReturn();
        $processing = TRUE;
      }
      else {
        if (strtolower($call->getCallChannel()) != 'text') {
          // for voice channel send a final hangup request
            $response .= '<Hangup/>';
            $msg = 'Script @name ended without a "hangup" command.';
            $params = array('@name' => $script->getName());
            watchdog('voipplivocore', $msg, $params, WATCHDOG_WARNING);
            $call->setHangupReason(VoipCall::HANGUP_SCRIPT_END);
        }
      }
      //for text channel don't do anything
    }

    // process current command
    else {
      $cmd_id = $cmd->getId();

      switch ($cmd_id) {
        default:
          $msg = "Invalid command id '@cmd_id' in script '@script_name': @script";
          $params = array(
            '@cmd_id' => $cmd_id,
            '@script_name' => $script->getName(),
            '@script' => print_r($script, TRUE)
          );
          $call->report_error('voipplivocore', $msg, $params);
          $rc = FALSE;
          $processing = FALSE;
          break;

        case 'VoipCmdBeep';
          $beep_url = url('voip/sound/beep', array('absolute' => TRUE));
          $response .= "<Play>$beep_url</Play>";
          break;

        case 'VoipCmdDial';
          $call->setCallStatus(VoipCall::IN_PROGRESS);
          $number = $script->evalString($cmd->getParam('number'));
          if (strtolower($call->getCallChannel()) == 'text') {
            //In case VoipCmdDial is called within text channel then instead of transfer we call the sender.
            //we are starting a call, hangup() the existing session
            $response .= '<Hangup/>';

            if (!$number) {
              $number = $call->getCallerNumber();
            }
            $caller_number = variable_get('voipcall_cid_number', NULL);
            $call->setDestNumber($number);
            $call->setCallerNumber($caller_number);
            $call->setScript($script);
            $call->save();
            voip_dial($call);
            $processing = FALSE;
          }
          else {
            $menu_path = 'voip/'.$server.'/callhandler';
            $cid = $call->getCid();
            $response_url = $menu_path . '/process_dial/' . $cid;
            $action = url($response_url, array('absolute' => TRUE));
            $method = 'POST';

            $timeout = $script->evalString($cmd->getParam('timeout', 30));
            $hangup_on_star = $script->evalString($cmd->getParam('hangup_on_star', FALSE));
            $hangup_on_star = $hangup_on_star ? 'true' : 'false';
            $time_limit = $script->evalString($cmd->getParam('time_limit', '14400'));
            $caller_number = $script->evalString($script->getVar('caller_number'));
            $caller_id = $script->evalString($cmd->getParam('caller_id', $caller_number));
            $response .= '<Dial action="' . $action
              . '" method="' . $method
              . '" hangupOnStar="' . $hangup_on_star
              . '" timeLimit="' . $time_limit
              . '" callerId="' . $caller_id .'"';
            if($server == 'plivocloud') {
              $response .=' timeout="' . $timeout .'"';
            }
            else {
              //No timeout in Plivo Framework dial.
            }
            $response .=  ' >';

            $response .= '<Number>' . $number . '</Number>';
            $response .= '</Dial>';
          }

          $rc = TRUE;
          $processing = FALSE;
          break;

        case 'VoipCmdGetInput':
          $call->setCallStatus(VoipCall::IN_PROGRESS);
          $menu_path = 'voip/'.$server.'/callhandler';
          $cid = $call->getCid();
          $response_url = $menu_path . '/process_get_input/' . $cid;
          $action = url($response_url, array('absolute' => TRUE));
          $method = 'POST';
          $timeout = $script->evalString($cmd->getParam('timeout', 5));
          if($server == 'plivocloud') {
            $end_key = $script->evalString($cmd->getParam('end_key', 'none'));
          }
          else {
            $end_key = $script->evalString($cmd->getParam('end_key', ''));
          }
          $num_digits = $script->evalString($cmd->getParam('num_digits'));
          $prompt = $cmd->getParam('prompt');
          $response .= '<GetDigits action="' . $action
            . '" method="' . $method
            . '" timeout="' . $timeout;

          if ($end_key) {
            $response .= '" finishOnKey="' . $end_key;
          }
		  if ($num_digits) {
            $response .= '" numDigits="' . $num_digits;
          }
          $response .= '" >';
          if ($prompt) {
            $response .= _voipplivocore_generate_audio_response($server, $script, $prompt);
          }
          $response .= '</GetDigits>';
          // in case of timeout or empty input, set Digits to 'no_input'
          $action_timeout = $action . '&amp;Digits=' . 'no_input';
          $response .= '<Redirect method="GET">'
            . $action_timeout
            . '</Redirect>';
          $rc = TRUE;
          $processing = FALSE;
          break;
        case 'VoipCmdGetVoiceInput':
          //TODO: http://www.plivo.org/docs/restxml/voice-elements/getspeech/
          $rc = TRUE;
          $processing = FALSE;
          break;

        case 'VoipCmdGosub':
          $script_name = $script->evalString($cmd->getParam('script_name'));
          $options = $script->evalString($cmd->getParam('options'));
          $rc = $script->runGosub($script_name, $options);
          if ($script->isError()) {
            $msg = $script->getErrorMessage();
            $params = array();
            $call->report_error('voipplivocore', $msg, $params);
            $rc = FALSE;
            $processing = FALSE;
            $found = TRUE;
            $text = 'Processing error. Please contact your system administrator.';
            $langcode = $script->getLanguage();
            $text = t($text, NULL, array('langcode' => $langcode));
            $response .= "<Speak>$text</Speak>";
          }
          break;

        case 'VoipCmdGoto';
          $label_name = $script->evalString($cmd->getParam('label_name', ''));
          $rc = $script->runGoto($label_name);
          if ($script->isError()) {
            $msg = $script->getErrorMessage();
            $params = array();
            $call->report_error('voipplivocore', $msg, $params);
            $rc = FALSE;
            $processing = FALSE;
            $found = TRUE;
            $text = 'Processing error. Please contact your system administrator.';
            $langcode = $script->getLanguage();
            $text = t($text, NULL, array('langcode' => $langcode));
            $response .= "<Speak>$text</Speak>";
          }
          break;

        case 'VoipCmdGotoIf';
          $condition = $script->evalString($cmd->getParam('condition', ''));
          if ($condition) {
            $label_name = $script->evalString($cmd->getParam('label_name', ''));
            $rc = $script->runGoto($label_name, $condition);
            if ($script->isError()) {
              $msg = $script->getErrorMessage();
              $params = array();
              $call->report_error('voipplivocore', $msg, $params);
              $rc = FALSE;
              $processing = FALSE;
              $found = TRUE;
              $text = 'Processing error. Please contact your system administrator.';
              $langcode = $script->getLanguage();
              $text = t($text, NULL, array('langcode' => $langcode));
              $response .= "<Speak>$text</Speak>";
            }
          }
          break;

        case 'VoipCmdHangup':
          $call->setCallStatus(VoipCall::IN_PROGRESS);
          $call->setHangupReason(VoipCall::HANGUP_SCRIPT_COMMAND);

          $hangup_mode = $script->evalString($cmd->getParam('hangup_mode'));

          switch ($hangup_mode) {
            case 'resume':
              if (strtolower($call->getCallChannel()) != 'text') {
                // send the hangup request and stop processing script commands
                // Note: this will trigger a response from Plivo with $plivo_action = process_hangup and CallStatus = completed
                $response .= '<Hangup />';
              }
              break;
            case 'reset':
              //Reset script
              $script->resetIndex();
              break;
            case 'end_session':
              //Kill the session if any
              $cid = $call->getCid();
              $session = VoipSession::getSessionByCid($cid);
              if ($session) {
                $session->destroy();
              }

              if (strtolower($call->getCallChannel()) != 'text') {
                // send the hangup request and stop processing script commands
                // Note: this will trigger a response from Plivo with $plivo_action = process_hangup and CallStatus = completed
                $response .= '<Hangup/>';
              }
              break;
          }


          $processing = FALSE;
          break;

        case 'VoipCmdJoinConference';
          // NOTE: Plivo offers several conference call parameters that are not
          //       supported by the VoIP Drupal API. In order to make those
          //       options available, the developer would have to extend the
          //       VoipScript class into something like VoipScriptPlivo.
          //       However, the specific commands of such class would not be
          //       supported by other VoIP servers.
          $call->setCallStatus(VoipCall::IN_PROGRESS);
          $menu_path = 'voip/'.$server.'/callhandler';
          $cid = $call->getCid();
          $response_url = $menu_path . '/process_conference/' . $cid;
          $action = url($response_url, array('absolute' => TRUE));
          $method = 'POST';
          $room = $script->evalString($cmd->getParam('room'));
          $muted = $script->evalString($cmd->getParam('muted', FALSE));
          $muted = $muted ? 'true' : 'false';
          $start_on_enter = 'true';
          $end_on_exit = 'false';
          $wait_url = ''; //waitSound in Plivo must return a RESTXML with Play and/or Wait Elements only
          $hangup_on_star = $script->evalString($cmd->getParam('hangup_on_star', FALSE));
          $hangup_on_star = $hangup_on_star ? 'true' : 'false';
          if($server == 'plivoframework') {
            $time_limit = '14400'; // max allowed for Plivo Framework calls
            $beep = '" beep="true';
          }
          else {
            $time_limit = '0';
            $beep = '" enterSound="beep:1';
          }

          $response .= '<Conference muted="' . $muted
            . $beep
            . '" startConferenceOnEnter="' . $start_on_enter
            . '" endConferenceOnExit="' . $end_on_exit
            . '" waitSound="' . $wait_url
            . '" hangupOnStar="' . $hangup_on_star
            . '" timeLimit="' . $time_limit
            . '" callbackUrl="' . $action
            . '" callbackMethod="' . $method
            . '" >';
          $response .= $room;
          $response .= '</Conference>';
          $rc = TRUE;
          $processing = FALSE;
          break;

        case 'VoipCmdLabel':
          // don't do anything. Labels are just placeholders that mark a position within the script.
          break;

        case 'VoipCmdLog':
          $input = $script->evalString($cmd->getParam('input', ''));
          $name = $script->evalString($cmd->getParam('name', NULL));
          $script->runLog($input, $name);
          break;

        case 'VoipCmdRecord':
          $call->setCallStatus(VoipCall::IN_PROGRESS);
          $menu_path = 'voip/'.$server.'/callhandler';
          $cid = $call->getCid();
          $format = $script->evalString($cmd->getParam('format'));
          $response_url = $menu_path . '/process_record/' . $cid;
          $action = url($response_url, array('absolute' => TRUE));
          $method = 'POST';
          $prompt = $cmd->getParam('prompt', '');
          $timeout = $script->evalString($cmd->getParam('timeout', 5));
          $end_key = $script->evalString($cmd->getParam('end_key', '#'));
          $max_length = $script->evalString($cmd->getParam('max_length', 3600));
          $play_beep = $script->evalString($cmd->getParam('play_beep'));
          $play_beep = $play_beep ? 'true' : 'false';
          if ($prompt) {
            $response .= _voipplivocore_generate_audio_response($server, $script, $prompt);
          }
          $response .= '<Record action="' . $action
            . '" method="' . $method
            . '" timeout="' . $timeout
            . '" finishOnKey="' . $end_key
            . '" maxLength="' . $max_length
            . '" playBeep="' . $play_beep
            . '" fileFormat="' . $format
            . '" />';
          // in case of empty recording...
          $response_url = $menu_path . '/process_empty_recording/' . $cid;
          $action = url($response_url, array('absolute' => TRUE));
          $response .= '<Redirect method="GET">'
            . $action
            . '</Redirect>';
          $rc = TRUE;
          $processing = FALSE;
          break;

        case 'VoipCmdReject':
          $call->setHangupReason(VoipCall::HANGUP_REJECTED);
          $reason_tmp = $script->evalString($cmd->getParam('reason'));
          $reason =
            ($reason_tmp == VoipScript::REJECT_BUSY) ? 'busy' : 'rejected';
          $response .= "<Hangup reason='$reason' />";
          $rc = TRUE;
          $processing = FALSE;
          break;

        case 'VoipCmdReturn':
          $rc = $script->runReturn();
          if ($script->isError()) {
            $msg = $script->getErrorMessage();
            $params = array();
            $call->report_error('voipplivocore', $msg, $params);
            $rc = FALSE;
            $processing = FALSE;
            $found = TRUE;
            // report the error to the phone user
            $voice_id = $script->getVoice();
            $voice = VoipVoice::getVoice($voice_id);
            $voice_id = _voipplivocore_get_plivo_voice($voice_id, $server);
            $text = 'Processing error. Please contact your system administrator.';
            // translate the text
            $langcode = $voice->getLanguage();
            $text = t($text, NULL, array('langcode' => $langcode));
            $response .= "<Speak voice='$voice_id' language='$language'>$text</Speak>";
          }
          break;

        case 'VoipCmdRunIvrMenu':
          $options['prompt'] = $cmd->getParam('prompt');
          $options['input_options'] =
            $script->evalString($cmd->getParam('input_options'));
          $options['invalid_msg'] = $cmd->getParam('invalid_msg');
          $options['no_input_msg'] = $cmd->getParam('no_input_msg');
          $options['max_attempts'] =
            $script->evalString($cmd->getParam('max_attempts'));
          $options['timeout'] = $script->evalString($cmd->getParam('timeout'));
          $rc = $script->runIvrMenu($options);
          break;

        case 'VoipCmdSay':
          $call->setCallStatus(VoipCall::IN_PROGRESS);
          $prompt = $cmd->getParam('prompt');
          $loop = $script->evalString($cmd->getParam('loop'));
          $response .= _voipplivocore_generate_audio_response($server, $script, $prompt, $loop);
          break;

        case 'VoipCmdSendText';
          $call->setCallStatus(VoipCall::IN_PROGRESS);
          $to = $script->evalString($cmd->getParam('to'));
          $text = $script->evalString($cmd->getParam('text'));
          $network = $script->evalString($cmd->getParam('network'));
          $sms_origin = $call->getCallOrigin();
          // If "to" is empty then send message to the person on the other
          // side of the phone. Depending on the direction of the call, that
          // might be either the caller or the callee.
          if (!$to) {
            $direction = $call->getDirection();
            if ($direction == 'inbound') {
              $to = $script->evalString($script->getVar('caller_number'));
              $from = $script->evalString($script->getVar('dest_number'));
            }
            else {
              $to = $script->evalString($script->getVar('dest_number'));
              $from = $script->evalString($script->getVar('caller_number'));
            }
            if (!$to) {
              watchdog('voipplivocore', 'Attempt to send ' . $network . ' to empty number', array(), WATCHDOG_ERROR);
              $processing = TRUE;
              break;
            }
          }

          if (voipscript_use_sms_framework()) {
            //Use SMS Framework to send SMS message if user enabled this option
            voipscript_send_sms_framework($to, $text);
          }
          else if ($sms_origin == 'smsframework') {
            //Use voip_text()
            $reply = TRUE;
            //We must unset dest_number because it will be number of SMS Framework gateway
            $call->setDestNumber('');
            $call = $call->save();
            voip_text($text, $call, $reply);
            $processing = TRUE;
          }
          else {
              if($server == 'plivoframework') {
                //Plivo Framework doesn't support SMS
                watchdog('voipplivoframework', 'Attempt to send an SMS without SMS Framework', array(), WATCHDOG_ERROR);
                $processing = TRUE;
              }
              else {
                $menu_path = 'voip/'.$server.'/callhandler';
                $cid = $call->getCid();
                $callback_url = $menu_path . '/process_sms_callback/' . $cid;
                $callback = url($callback_url, array('absolute' => TRUE));
                $method = 'POST';

                if(!$from) {
                // Note: If no caller number provided, Twilio will automatically try
                //       to determine an appropriate value depending on whether the
                //       call is inbound or outbound. However, since that number might
                //       not be sms capable, it might incur in a processing error.
                // Use the system's number as the default caller id
                  $default_cid_number = variable_get('voipcall_cid_number', NULL);
                  $from = $script->evalString($cmd->getParam('caller_id', $default_cid_number));
                }
                $rc = TRUE;
                switch (strtolower($network)) {
                  default:
                    $params = array(
                      '@network' => $network,
                      '@script_name' => $script->getName(),
                      '@script' => print_r($script, TRUE)
                    );
                    $msg = "Invalid 'network' parameter '@network' for SendText command in script '@script_name': @script";
                    $call->report_error('voip'.$server, $msg, $params);
                    $rc = FALSE;
                    $processing = FALSE;
                    break;

                  case 'sms':
                    $response .= '<Message callbackUrl="' . $callback
                      . '" callbackMethod="' . $method;
                    if ($to) {
                      $response .= '" dst="' . $to;
                    }
                    if ($from) {
                      $response .= '" src="' . $from;
                    }
                    $response .= '" >';
                    // replace special chars for their html equivalent (http://www.twilio.com/docs/errors/12100)
                    $response .= htmlspecialchars($text, ENT_NOQUOTES | ENT_HTML401);
                    $response .= '</Message>';
                    // since the 'action' parameter prevents other commands from
                    // being executed, set $processing to FALSE
                    $processing = FALSE;
                    break;
                }
              }
          }
          break;

        case 'VoipCmdSet';
          $var_name = $script->evalString($cmd->getParam('var_name'));
          $var_value = $script->evalString($cmd->getParam('var_value'));
          $script->setVar($var_name, $var_value);
          break;

        case 'VoipCmdSetVoice';
          $voice_id = $script->evalString($cmd->getParam('voice'));
          $voice = VoipVoice::getVoice($voice_id);
          $voice_id = $voice->getVoiceId();
          $script->setVoice($voice_id);
          break;

        case 'VoipCmdUnset';
          $var_name = $script->evalString($cmd->getParam('var_name'));
          $script->unsetVar($var_name);
          break;

        case 'VoipCmdWait';
          $time_limit = $script->evalString($cmd->getParam('time_limit'));
          $menu_path = 'voip/'.$server.'/callhandler';
          $cid = $call->getCid();
          $response_url = $menu_path . '/process_wait/' . $cid;
          $redirect_url = url($response_url, array('absolute' => TRUE));
          $response .= '<Wait length="' . $time_limit . '"/><Redirect>' . $redirect_url . '</Redirect>';
          $processing = FALSE;
          break;

      }

      // go to next command
    }
  }

  // save script for next iteration
  $call->setScript($script);
  $call->save();

  // send response back to Plivo
  if ($response) {

//$response .= '<!--' . "_REQUEST: " . print_r($_REQUEST,true) .  "\n" . '-->';
//$response .= '<!--' . "voipcall: " . print_r($call,true) .  "\n" . '-->';

    _voipplivocore_send_response($response);
  }

  return $rc;
}

/**
 * Send responses to Plivo
 */
function _voipplivocore_send_response($response) {
  $output = '<?xml version="1.0" encoding="UTF-8" ?>';
  $output .= '<Response>';
  $output .= $response;
  $output .= '</Response>';
  drupal_add_http_header('Content-Type', 'text/xml; charset=utf-8');
  print $output;
  return TRUE;
}


/**
 * Update 'call status' and 'hangup reason' based on Plivo's response
 */
function voipplivocore_update_call_status($call, $plivo_call_status = 'in-progress') {
  switch ($plivo_call_status) {
    default:
      $msg = 'Invalid CallStatus: @status for call cid: @cid';
      $cid = $call->getCid();
      $params = array('@status' => $plivo_call_status, '@cid' => $cid);
      $call->report_error('voipplivocore', $msg, $params);
      return NULL;
      break;
    case 'failed': // call failed, most likely due to invalid number
      $call->setCallStatus(VoipCall::INVALID);
      $call->setHangupReason(VoipCall::HANGUP_INVALID_NUMBER);
      break;
    case 'queued':
      $call->setCallStatus(VoipCall::QUEUED);
      break;
    case 'ringing': //verified
      $call->setCallStatus(VoipCall::RINGING);
      break;
    case 'in-progress': //verified
      $call->setCallStatus(VoipCall::IN_PROGRESS);
      break;
    case 'completed': //verified
      $call->setCallStatus(VoipCall::COMPLETED);
      $hangup_reason = $call->getHangupReason();
      if (!$hangup_reason) {
        $call->setHangupReason(VoipCall::HANGUP_BY_USER);
      }
      break;
    case 'busy':
      $call->setCallStatus(VoipCall::BUSY);
      $call->setHangupReason(VoipCall::HANGUP_BUSY);
      break;
    case 'no-answer':
      $call->setCallStatus(VoipCall::NO_ANSWER);
      $call->setHangupReason(VoipCall::HANGUP_NO_ANSWER);
      break;
    case 'canceled':
    case 'cancel':
      $call->setCallStatus(VoipCall::CANCELED);
      $call->setHangupReason(VoipCall::HANGUP_API_REQUEST);
      break;
  }
  return $call;
}


/**
 * Process the response to a "dial" script command
 */
function _voipplivocore_process_dial_response($server, $call, $request) {
  $script = $call->getScript();

  // update the 'dial_sid' script variable
  if($server =='plivoframework') {
    $script->setVar('dial_sid', $request['DialCallSid']);
  }
  else {
    $script->setVar('dial_sid', $request['DialBLegUUID']);
  }


  // update the 'dial_status' script variable
  if($server =='plivoframework') {
    $dial_status = $request['DialCallStatus'];
  }
  else {
    $dial_status = $request['DialBLegStatus'];
  }
  switch ($dial_status) {
    default:
      $cid = $call->getCid();
      $msg = 'Invalid DialCallStatus: @status for call cid: @cid';
      $params = array('@status' => $dial_status, '@cid' => $cid);
      $call->report_error('voipplivocore', $msg, $params);
      return NULL;
      break;
    case 'answered':
    case 'completed':
    case 'answer':
      $script->setVar('dial_status', VoipScript::DIAL_COMPLETED);
      break;
    case 'busy':
      $script->setVar('dial_status', VoipScript::DIAL_BUSY);
      break;
    case 'no-answer':
    case 'hangup':
      $script->setVar('dial_status', VoipScript::DIAL_NO_ANSWER);
      break;
    case 'failed':
      $script->setVar('dial_status', VoipScript::DIAL_FAILED);
      break;
    case 'canceled':
      $script->setVar('dial_status', VoipScript::DIAL_CANCELED);
      break;
  }
  // update the 'dial_duration' script variable
  if($server =='plivoframework') {
    $duration = $request['DialCallDuration'];
  }
  else {
    $duration = $request['DialBLegDuration'];
  }

  if (isset($duration)) {
    $script->setVar('dial_duration', $duration);
  }

  $call->setScript($script);
  return $call;
}

/**
 * Process the response to a 'send text' script command
 */
function _voipplivocore_process_sms_response($call, $request){
  $cid = $call->getCid();
  $sms_status_tmp = $request['Status'];
  switch($sms_status_tmp) {
    default:
      $msg = 'Invalid SmsStatus: @status for call cid: @cid';
      $params = array('@status' => $sms_status_tmp, '@cid' => $cid);
      $call->report_error('voipplivocore', $msg, $params);
      return NULL;
      break;
    case 'failed':
      $msg = 'SMS sending failed for call cid: @cid. Make sure your Plivo number is sms-enabled.';
      $params = array('@cid' => $cid);
      watchdog('voipplivocore', $msg, $params, WATCHDOG_WARNING);
      $text_status = VoipCall::TEXT_FAILED;
      break;
    case 'invalid':
      $msg = 'Invalid SMS parameter for call cid: @cid';
      $params = array('@cid' => $cid);
      watchdog('voipplivocore', $msg, $params, WATCHDOG_WARNING);
      $text_status = VoipCall::TEXT_INVALID;
      break;
    case 'sending':
      $text_status = VoipCall::TEXT_SENDING;
      break;
    case 'received':
    case 'sent':
      $text_status = VoipCall::TEXT_SENT;
      break;
  }
  $call->setTextStatus($text_status);
  $script = $call->getScript();
  $script->setVar('send_text_status', $text_status);
  $call->setScript($script);
  return $call;
}

define('voipplivocore_URL_CHECK', '#(https?://\S+(?<![,.;?\:\!]))#');

/**
 * Generate a prompt to be played to the user
 *
 * @param $script
 *   The VoipScript instance being executed
 *
 * @param $prompt
 *   Mixed. Can be any of the following
 *   - an executable string to be processed into a prompt string, a VoipPrompt
 *     instance, or an array of VoipPrompt instances
 *   - a prompt string to played to the user
 *   - an array of VoipPrompt instances to be processed
 *
 * @param $loop
 *   Integer with the number of times to repeat the prompt
 *
 * @return
 *   The prompt as a PlivoML sequence to be sent to Plivo
 */
function _voipplivocore_generate_audio_response($server, $script, $prompt, $loop = 1) {
  $response = '';
  if (is_string($prompt) && (substr($prompt, 0, 1) == '^')) {
    $prompt = $script->evalString($prompt);
  }

  if (is_string($prompt)) {
    $prompt_list[] = new VoipPrompt($prompt);
  }
  else {
    if (is_object($prompt)) {
      $prompt_list[] = $prompt;
    }
    else {
      $prompt_list = $prompt;
    }
  }

  $default_voice_id = $script->getVoice();

  foreach ($prompt_list as $p) {
    $text = $script->evalString($p->getText());
    //encoding the sentence to handle special characters like in some foreign languages
    $convmap = array(0x80, 0xffff, 0, 0xffff);
    $text = mb_encode_numericentity($text, $convmap, 'UTF-8');
    $voice_id = $p->getVoice() ? $p->getVoice() : $default_voice_id;
    $voice = VoipVoice::getVoice($voice_id);
    $gender = $voice->getGender();
    $language = $voice->getLanguage();

    //we need to get safe voice id
    $voice_id = _voipplivocore_get_plivo_voice($voice_id, $server);

    $as_tmp = $p->getAs() ? $script->evalString($p->getAs()) : '';
    $as = $as_tmp ? $as_tmp : 'number';

    // split the text into audio URLs and text elements
    $elements =
      preg_split(voipplivocore_URL_CHECK, $text, -1, PREG_SPLIT_DELIM_CAPTURE);

    // process each element
    $url_flag = FALSE;
    foreach ($elements as $element) {
      $element = trim($element);
      if ($element) {
        if (_voipplivocore_is_url($element)) {
          $response .= "<Play>$element</Play>";
          $url_flag = TRUE;
        }
        else {
          if ($url_flag) {
            // remove punctuation associated with the previous url element
            // i.e. the '!' in $text = 'Hello $audio_name!"
            $element = ltrim($element, '?!.;,:');
            $url_flag = FALSE;
          }
          if ($element) {
            if ($as == 'digits') {
              //Convert numbers into digits
              $element = _voipplivocore_parse_numbers($element);
            }
            if($server == 'plivocloud') {
              //Plivo cloud uses gender as voice id (man/woman)
              $speak_voice = strtoupper($gender);
              //Plivo cloud uses combination of language with region id
              $speak_language = VoipPlivoCloudServer::getLangRegionId($voice_id);
            }
            else {
              //Plivo Framework uses voice id as voice id (kal, rms,...)
              $speak_voice = $voice_id;
              $speak_language = $language;
            }
            $response .= "<Speak voice='$speak_voice' language='$speak_language'>$element</Speak>";
          }
        }
      }
    }
  }

  // process the $loop parameter
  $counter = $loop - 1;
  while ($counter > 0) {
    $response .= $response;
    $counter--;
  }

  return $response;
}


/**
 * Check if the given string is a url
 */
function _voipplivocore_is_url($str) {
  // Note: Although the checking performed is not as complete as
  //       http://stackoverflow.com/questions/161738/what-is-the-best-regular-expression-to-check-if-a-string-is-a-valid-url , it seems to serve our purposes!
  $rc = preg_match(voipplivocore_URL_CHECK, $str);
  return $rc;
}

function _voipplivocore_parse_numbers($str) {
//preg_match_all("{(\d+)}", $str, $result, PREG_PATTERN_ORDER);
//return $result;
  return preg_replace_callback('{(\d+)}', '_voipplivocore_numbers_add_space', $str);
}

function _voipplivocore_numbers_add_space($matches) {
  $string_array = str_split($matches[0]);
  $number_with_space = implode(" ", $string_array);
  return $number_with_space;
}
/*
function _voipplivocore_transfer_file($url) {
  $dest = file_default_scheme() . '://' ;
  $filename = basename($url);
  $file_destination = file_destination($dest . $filename, FILE_EXISTS_RENAME);
  //File is not immediately available in plivo for download
  $i = 0;
  $data = file_get_contents($url);
  while($data === FALSE && $i<=100) {
    $data = file_get_contents($url);
    sleep(1);
    $i++;
  }
  $file = file_save_data($data, $file_destination);
  return $file;
}*/

function _voipplivocore_get_plivo_voice($voice_id, $server) {
  $default_server = VoipServer::getServer($server);
  $plivo_voices = $default_server->_get_available_voices();
  if (!isset($plivo_voices[$voice_id])) {
    if ($gender == 'man') {
      $voice_id = $default_server->getDefaultManVoice()->getVoiceId();
    }
    else if ($gender == 'woman') {
      $voice_id = $default_server->getDefaultWomanVoice()->getVoiceId();
    }
    else {
      $voice_id = $default_server->getDefaultVoice()->getVoiceId();
    }
  }
  return $voice_id;
}

function _voipplivocore_file_save_upload($audio_url){
  $dest = file_default_scheme() . '://' ;
  $filename = basename($audio_url);
  $file_destination = file_destination($dest . $filename, FILE_EXISTS_RENAME);
  //File is not immediately available in plivo for download
  $i = 0;
  $data = file_get_contents($audio_url);
  while($data === FALSE && $i<=100) {
    $data = file_get_contents($audio_url);
    sleep(1);
    $i++;
  }
  $file = file_save_data($data, $file_destination);
  return $file;
}
