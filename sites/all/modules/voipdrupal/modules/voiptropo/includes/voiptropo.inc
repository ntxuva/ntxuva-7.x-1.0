<?php
// $id$

/**
 * @file
 * Implementation of tropo-related functionality
 */

/**
 * Internal functions
 */
/*
 * Process incoming requests and responses from tropo
 *
 * @param string $tropo_action set to the action to be executed.
 *   It can assume any of the following values:
 *   - process_conference, to handle responses to conference call requests
 *   - process_inbound_calls, to handle incoming calls
 *   - process_outbound_calls, to handle the response to outgoing call requests
 *   - process_dial, to handle the response to the "dial" script command
 *   - process_get_input, to handle responses to "get input" script commands
 *   - process_hangup, to handle call termination
 *   - process_record, process_record_mp3 and process_empty_recording, to handle
 *     responses to 'record' script commands
 *   - process_wait, to continue after wait command
 *   - process_send_sms, to handle responses to 'send sms' script commands
 *   - process_sms_callback, to handle final responses to 'send sms'.
 *   - process_outbound_text, to send new text message
 *
 * @param string $cid with the id of the call associated with the response
 *
 * @return void
 */
function _voiptropo_call_handler($tropo_action = '', $cid = NULL, $event_type = '') {
  if (voip_debug()) {
    $msg = "in voiptropo_call_handler() with tropo_action: $tropo_action, cid: $cid, event: $event_type";
    watchdog('voiptropo', $msg);
  }
  // TODO: How to validate requests to make sure they are from Tropo?
  //$server = VoipServer::getServer('tropo');
  //$server_config = $server->getConfig();

  // process the special case of save_record, which does not have a json
  // object associated with it
  if ($tropo_action == 'save_record') {
      $call = VoipCall::load($cid);
      $script = $call->getScript();
      $script->setVar('recording_digits', NULL);

    //@todo: move file
    $file = _voiptropo_file_save_upload();
    $script->setVar('recording_public_url', file_create_url($file->uri));
    $script->setVar('recording_fid', $file->fid);
    $call->setScript($script);
    // update the voipcall with the new values
    $call = $call->save();
    return;
  }

  // determine tropo_action and cid based on the incoming session object
  try {
    // attempt to handle new outbound and inbound voice/text calls
    $session = new Session();

    $tropo_action = empty($tropo_action) ? $session->getParameters('tropo_action') : $tropo_action;
    if (empty($tropo_action)) {
      $tropo_action = 'process_inbound_calls';
    }

    $cid = $session->getParameters('cid');
    // process call requests
    $call = _voiptropo_handle_new_calls($tropo_action, $session);
  } catch (TropoException $e) {
    try {
      // attempt to process responses/signals/events from ongoing calls
      $result = new Result();
      $call = _voiptropo_handle_ongoing_calls($result, $tropo_action, $cid, $event_type);
    } catch (TropoException $e) {
      watchdog('voiptropo', 'error in _voiptropo_call_handler, Tropo reported: ' . $e->getMessage(), array(), WATCHDOG_ERROR);
      return FALSE;
    }
  }

  if ($call) {
    // get ready to process the call script
    $tropo = new Tropo();
    $menu_path = 'voip/tropo/callhandler';

    // process the call script
    $rc = _voiptropo_script_handler($call, $tropo);

    // add common event triggers
    $channel = $call->getCallChannel();
    if ($tropo_action != 'process_callstatus' && $channel != 'text') {
      $cid = $call->getCid();
      $response_url = $menu_path . '/process_callstatus/' . $cid . '/error';
      $action = url($response_url, array('absolute' => TRUE));
      $tropo->on(array("event" => "error", "next" => $action));
    }

    // send response back to tropo
    $tropo->renderJSON();
  }

  return;
}

/**
 * Process voipcall scripts
 *
 * @param $call, VoipCall instance with the call information
 *
 * @return boolean.  TRUE on success or FALSE on failure.  Check voip_error() for processing failure.
 */
function _voiptropo_script_handler($call, &$tropo) {
  if (voip_debug()) {
    $msg = 'Entering script_handler with voipcall: ' . print_r($call, TRUE);
    watchdog('voiptropo', $msg, array(), WATCHDOG_INFO);
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

  $processing = TRUE;
  $response = '';

  //Set script language
  $language = $script->getLanguage();
  $voice = $script->getVoice();
  $tropo->setVoice($voice);

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
        if ($call->getCallChannel() != 'text') {
          // for voice channel send a final hangup request
          $tropo->hangup();
          $msg = 'Script @name ended without a "hangup" command.';
          $params = array('@name' => $script->getName());
          watchdog('voiptropo', $msg, $params, WATCHDOG_WARNING);
          $call->SetCallStatus(VoipCall::COMPLETED);
          $call->setHangupReason(VoipCall::HANGUP_SCRIPT_END);
          $call->setEndTime();
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
            '@script' => print_r($script, TRUE),
          );
          $call->report_error('voiptropo', $msg, $params);
          $rc = FALSE;
          $processing = FALSE;
          break;

        case 'VoipCmdDial':
          $call->setCallStatus(VoipCall::IN_PROGRESS);
          $number = $script->evalString($cmd->getParam('number'));
          $number = str_replace(" ", "", $number); //remove any spaces from number
          if (strtolower($call->getCallChannel()) == 'text') {
            //In case VoipCmdDial is called within text channel then instead of transfer we call the sender.
            //we are starting a call, hangup() the existing session
            $tropo->hangup();

            if (!$number) {
              $number = $call->getCallerNumber();
            }
            //$call = new VoipCall();
            $call->setDestNumber($number);
            $call->setCallerNumber(variable_get('voipcall_cid_number', ''));
            $call->setScript($script);
            $call->save();
            voip_dial($call);
          }
          else {
            $menu_path = 'voip/tropo/callhandler';
            $cid = $call->getCid();
            $timeout = $script->evalString($cmd->getParam('timeout', 30));
            //TODO: What command for timeLimit, hangupOnStar?
            /*$hangup_on_star =  $script->evalString($cmd->getParam('hangup_on_star', false));
             $hangup_on_star =  $hangup_on_star? 'true':'false';
             $time_limit = $script->evalString($cmd->getParam('time_limit', '14400'));*/
            $caller_number = $script->evalString($script->getVar('caller_number'));
            $caller_id = $script->evalString($cmd->getParam('caller_id', $caller_number));

            $params = array(
              //'timeout' => $timeout,  //TODO: Timeout causes immediate hangup, disabled for now.
              'from' => $caller_id,
            );
            $tropo->transfer($number, $params);

            //Add events
            //Add events
            $response_url = $menu_path . '/process_dial/' . $cid . '/continue';
            $action = url($response_url, array('absolute' => TRUE));
            $tropo->on(array("event" => "continue", "next" => $action));

            $response_url = $menu_path . '/process_dial/' . $cid . '/error';
            $action = url($response_url, array('absolute' => TRUE));
            $tropo->on(array("event" => "error", "next" => $action));

            $response_url = $menu_path . '/process_dial/' . $cid . '/incomplete';
            $action = url($response_url, array('absolute' => TRUE));
            $tropo->on(array("event" => "incomplete", "next" => $action));

            $response_url = $menu_path . '/process_hangup/' . $cid;
            $action = url($response_url, array('absolute' => TRUE));
            $tropo->on(array("event" => "hangup", "next" => $action));
          }

          $rc = TRUE;
          $processing = FALSE;
          break;

        case 'VoipCmdGetInput':
          _voiptropo_get_input('dtmf', $call, $cmd, $script, $tropo);
          $rc = TRUE;
          $processing = FALSE;
          break;

        case 'VoipCmdGetVoiceInput':
          _voiptropo_get_input('voice', $call, $cmd, $script, $tropo);
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
            $call->report_error('voiptropo', $msg, $params);
            $rc = FALSE;
            $processing = FALSE;
            $found = TRUE;
            $text = 'Processing error. Please contact your system administrator.';
            $langcode = $script->getLanguage();
            $text = t($text, NULL, array('langcode' => $langcode));
            $tropo->say($text);
          }
          break;

        case 'VoipCmdGoto':
          $label_name = $script->evalString($cmd->getParam('label_name', ''));
          $rc = $script->runGoto($label_name);
          if ($script->isError()) {
            $msg = $script->getErrorMessage();
            $params = array();
            $call->report_error('voiptropo', $msg, $params);
            $rc = FALSE;
            $processing = FALSE;
            $found = TRUE;
            $text = 'Processing error. Please contact your system administrator.';
            $langcode = $script->getLanguage();
            $text = t($text, NULL, array('langcode' => $langcode));
            $tropo->say($text);
          }
          break;

        case 'VoipCmdGotoIf':
          $condition = $script->evalString($cmd->getParam('condition', ''));
          if ($condition) {
            $label_name = $script->evalString($cmd->getParam('label_name', ''));
            $rc = $script->runGoto($label_name, $condition);
            if ($script->isError()) {
              $msg = $script->getErrorMessage();
              $params = array();
              $call->report_error('voiptropo', $msg, $params);
              $rc = FALSE;
              $processing = FALSE;
              $found = TRUE;
              $text = 'Processing error. Please contact your system administrator.';
              $langcode = $script->getLanguage();
              $text = t($text, NULL, array('langcode' => $langcode));
              $tropo->say($text);
            }
          }
          break;

        case 'VoipCmdHangup':
          $call->setCallStatus(VoipCall::COMPLETED);
          $call->setHangupReason(VoipCall::HANGUP_SCRIPT_COMMAND);
          $start = $call->getStartTime();
          $end = $call->setEndTime();
          $duration = $end - $start;
          $call->setDuration($duration);
          $hangup_mode = $script->evalString($cmd->getParam('hangup_mode'));

          switch ($hangup_mode) {
            case 'resume':
              // don't do anything
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
              break;
          }
          if ($call->getCallChannel() != 'text') {
            // send the hangup request and stop processing script commands
            $tropo->hangup();
          }
          $processing = FALSE;
          break;

        case 'VoipCmdJoinConference':
          $call->setCallStatus(VoipCall::IN_PROGRESS);
          $menu_path = 'voip/tropo/callhandler';
          $cid = $call->getCid();
          $response_url = $menu_path . '/process_conference/' . $cid;
          $action = url($response_url, array('absolute' => TRUE));

          $room = $script->evalString($cmd->getParam('room'));
          $muted = $script->evalString($cmd->getParam('muted', FALSE));
          $muted = $muted ? 'true' : 'false';

          $wait_url = $script->evalString($cmd->getParam('wait_url', ''));
          $hangup_on_star = $script->evalString($cmd->getParam('hangup_on_star', FALSE));
          if ($hangup_on_star) {
            $terminator = "*";
          }
          else {
            $terminator = "";
          }
          //TODO: What about the following Twilio options: beep, start_on_enter, end_on_exit, time_limit?
          $params = array(
            'id' => $room,
            'mute' => $muted,
            'terminator' => $terminator,
          );

          $tropo->conference($room, $params);
          $tropo->on(array("event" => "continue", "next" => $action));

          $response_url = $menu_path . '/process_hangup/' . $cid;
          $action = url($response_url, array('absolute' => TRUE));
          $tropo->on(array("event" => "hangup", "next" => $action));

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
          $menu_path = 'voip/tropo/callhandler';
          $cid = $call->getCid();
          $format = $script->evalString($cmd->getParam('format'));
          if ($format == 'mp3') {
            $record_format = 'audio/mp3';
          }
          else {
            // return wav file
            $record_format = 'audio/wav';
          }
          $timeout = $script->evalString($cmd->getParam('timeout', 5.0));
          $max_length = $script->evalString($cmd->getParam('max_length', 3600));

          $play_beep = $script->evalString($cmd->getParam('play_beep'));
          $play_beep = $play_beep ? 'true' : 'false';

          $response_url = $menu_path . '/save_record/' . $cid;
          $record_action = url($response_url, array('absolute' => TRUE));

          $end_key = $script->evalString($cmd->getParam('end_key', '#'));
          $choices = $end_key;

          $params = array(
            'maxTime' => $max_length,
            'maxSilence' => (float) $timeout,
            'beep' => $play_beep,
            'format' => $record_format,
            'method' => 'POST',
            'url' => $record_action,
            'choices' => $choices,
          );

          $prompt = _voiptropo_prompt_validate($script, $cmd->getParam('prompt'));
          $default_language = $script->getLanguage();
          $default_voice = $script->getVoice();

          /*bug with Tropo in VoipCmdRecord. If say parameter is missing from tropo record() command, an error will happen terminating
            the current call. Therefore getting last item in array and moving it to record() command.*/
          $last_say = array_pop($prompt);
          $text = $script->evalString($last_say->getText());
          $text = _voiptropo_strip_special_chars($text);
          $voice_id = $last_say->getVoice() ? $last_say->getVoice() : $default_voice;

          //we need to get safe voice id
          $voice_id = _voiptropo_get_tropo_voice_id($voice_id);

          $as = $last_say->getAs() ? $script->evalString($last_say->getAs()) : '';

          if (!empty($as) && in_array($as, array('digits', 'date'))) {
            $params['as'] = $as;
          }
          $params['voice'] = $voice_id;
          $params['say'] = $text;

          foreach ($prompt as $p) {
            $text = $script->evalString($p->getText());
            $text = _voiptropo_strip_special_chars($text);
            $voice_id = $p->getVoice() ? $p->getVoice() : $default_voice;
            $voice_id = _voiptropo_get_tropo_voice_id($voice_id);
            $as = $p->getAs() ? $script->evalString($p->getAs()) : '';
            $say_params = array();
            if (!empty($as) && in_array($as, array('digits', 'date'))) {
              $say_params['as'] = $as;
            }
            $say_params['voice'] = $voice_id;
            //Solution for Tropo error with sprintf and url encoded strings
            //https://www.tropo.com/bizblog/viewer?&bb-name=tropo_support&bb-q=sprintf&&bb-cid=99&bb-tid=1193463#bb
            $text = str_replace("%", "%%", $text);
            $tropo->say($text, $say_params);
          }

          $tropo->record($params);

          //What next:
          $response_url = $menu_path . '/process_record/' . $cid;
          $success_action = url($response_url, array('absolute' => TRUE));
          $tropo->on(array("event" => "continue", "next" => $success_action));
          // in case of empty recording...
          $response_url = $menu_path . '/process_empty_recording/' . $cid;
          $timeout_action = url($response_url, array('absolute' => TRUE));
          $tropo->on(array("event" => "incomplete", "next" => $timeout_action));

          $response_url = $menu_path . '/process_record_hangup/' . $cid;
          $hangup_action = url($response_url, array('absolute' => TRUE));
          $tropo->on(array("event" => "hangup", "next" => $hangup_action));

          $rc = TRUE;
          $processing = FALSE;
          break;

        case 'VoipCmdReject':
          $call->SetCallStatus(VoipCall::REJECTED);
          $call->setHangupReason(VoipCall::HANGUP_REJECTED);
          $start = $call->getStartTime();
          $end = $call->setEndTime();
          $duration = $end - $start;
          $call->setDuration($duration);
          $tropo->reject();
          // subscribe to events
          $menu_path = 'voip/tropo/callhandler';
          $cid = $call->getCid();
          $response_url = $menu_path . '/process_hangup/' . $cid;
          $action = url($response_url, array('absolute' => TRUE));
          $tropo->on(array("event" => "hangup", "next" => $action));
          $rc = TRUE;
          $processing = FALSE;
          break;

        case 'VoipCmdReturn':
          $rc = $script->runReturn();
          if ($script->isError()) {
            $msg = $script->getErrorMessage();
            $params = array();
            $call->report_error('voiptropo', $msg, $params);
            $rc = FALSE;
            $processing = FALSE;
            //$found = TRUE;
            // report the error to the phone user
            $text = 'Processing error. Please contact your system administrator.';
            // translate the text
            $langcode = $script->getLanguage();
            $text = t($text, NULL, array('langcode' => $langcode));
            $tropo->say($text);
          }
          break;

        case 'VoipCmdRunIvrMenu':
          $options['prompt'] = $cmd->getParam('prompt');
          $options['input_options'] = $script->evalString($cmd->getParam('input_options'));
          $options['invalid_msg'] = $cmd->getParam('invalid_msg');
          $options['no_input_msg'] = $cmd->getParam('no_input_msg');
          $options['max_attempts'] = $script->evalString($cmd->getParam('max_attempts'));
          $options['timeout'] = $script->evalString($cmd->getParam('timeout'));
          $rc = $script->runIvrMenu($options);
          break;

        case 'VoipCmdSay':
          $call->setCallStatus(VoipCall::IN_PROGRESS);
          $loop = $script->evalString($cmd->getParam('loop'));
          $prompt = _voiptropo_prompt_validate($script, $cmd->getParam('prompt'));
          $default_language = $script->getLanguage();
          $default_voice = $script->getVoice();

          $menu_path = 'voip/tropo/callhandler';
          $cid = $call->getCid();
          $response_url = $menu_path . '/process_hangup/' . $cid;
          $action = url($response_url, array('absolute' => TRUE));

          for ($i = 0; $i < $loop; $i++) {
            foreach ($prompt as $p) {
              $text = $script->evalString($p->getText());
              $text = _voiptropo_strip_special_chars($text);
              $voice_id = $p->getVoice() ? $p->getVoice() : $default_voice;
              //we need to get safe voice id
              $voice_id = _voiptropo_get_tropo_voice_id($voice_id);
              $as = $p->getAs() ? $script->evalString($p->getAs()) : '';
              $say_params = array();
              if ($as == 'digits') {
                $text = "<speak>" . _voiptropo_parse_numbers($text) . "</speak>";
              }
              elseif ($as == 'date') {
                $say_params['as'] = $as;
              }

              $say_params['voice'] = $voice_id;
              //Solution for Tropo error with sprintf and url encoded strings
              //https://www.tropo.com/bizblog/viewer?&bb-name=tropo_support&bb-q=sprintf&&bb-cid=99&bb-tid=1193463#bb
              $text = str_replace("%", "%%", $text);
              $tropo->say($text, $say_params);

              $tropo->on(array("event" => "hangup", "next" => $action));
            }
          }
          break;

        case 'VoipCmdSendText':
          $call->setCallStatus(VoipCall::IN_PROGRESS);
          $to = $script->evalString($cmd->getParam('to'));
          //If "to" is empty then send message to the caller
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
              watchdog('voiptropo', 'Attempt to send ' . $network . ' to empty number', array(), WATCHDOG_ERROR);
              $processing = TRUE;
              break;
            }
          }
          $text = $script->evalString($cmd->getParam('text'));
          $network = $script->evalString($cmd->getParam('network'));
          $sms_origin = $call->getCallOrigin();

          if (voipscript_use_sms_framework()) {
            //Use SMS Framework to send SMS message if user enabled this option
            voipscript_send_sms_framework($to, $text);
          }
          else {
            if ($sms_origin == 'smsframework') {
              //Use voip_text()
              $reply = TRUE;
              //We must unset dest_number because it will be number of SMS Framework gateway
              $call->setDestNumber('');
              $call = $call->save();
              voip_text($text, $call, $reply);

              $call = $call->save();
            }
            else {
              //Get from number if not set
              if (!$from && strtolower($network) == 'sms') {
                //We need caller id only for sms
                // Note: If no caller number provided, use the system's number as the default caller id
                $default_cid_number = variable_get('voipcall_cid_number', NULL);
                $caller_id = $script->evalString($cmd->getParam('caller_id', $default_cid_number));
                $from = $caller_id;
              }
              $params = array(
                'from' => $from,
                'to' => $to,
                'channel' => 'TEXT',
                'network' => $network,
              );
              $tropo->message($text, $params);
              //Register events:
              $menu_path = 'voip/tropo/callhandler';
              $cid = $call->getCid();

              $callback_url = $menu_path . '/process_sms_callback/' . $cid;
              //$callback_continue = url($callback_url . '/continue', array('absolute' => TRUE));
              //$tropo->on(array("event" => "continue", "next" =>  $callback_continue));

              $callback_error = url($callback_url . '/error', array('absolute' => TRUE));
              $tropo->on(array("event" => "error", "next" => $callback_error));

              $callback_incomplete = url($callback_url . '/incomplete', array('absolute' => TRUE));
              $tropo->on(array(
                "event" => "incomplete",
                "next" => $callback_incomplete
              ));

              //$callback_hangup = url($callback_url . '/hangup', array('absolute' => TRUE));
              //$response_url = $menu_path . '/process_hangup/' . $cid ;
              //$action = url($response_url, array('absolute' => TRUE));
              //$tropo->on(array("event" => "hangup", "next" => $action));
            }
          }
          $rc = TRUE;
          $processing = TRUE;
          break;

        case 'VoipCmdBeep':
          $beep_url = url('voip/sound/beep', array('absolute' => TRUE));
          $tropo->say($beep_url);
          break;

        case 'VoipCmdSet':
          $var_name = $script->evalString($cmd->getParam('var_name'));
          $var_value = $script->evalString($cmd->getParam('var_value'));
          $script->setVar($var_name, $var_value);
          break;

        case 'VoipCmdSetVoice':
          $voice_id = $script->evalString($cmd->getParam('voice'));
          $voice = VoipVoice::getVoice($voice_id);
          $voice_id = $voice->getVoiceId();
          $script->setVoice($voice_id);
          break;

        case 'VoipCmdUnset':
          $var_name = $script->evalString($cmd->getParam('var_name'));
          $script->unsetVar($var_name);
          break;

        case 'VoipCmdWait':
          $channel = $call->getCallChannel();
          if ($channel != 'voice') {
            watchdog('voiptropo', 'Attempt to call addWait() command in SMS message.', array(), WATCHDOG_ERROR);
            $processing = TRUE;
            break;
          }
          $time_limit = $script->evalString($cmd->getParam('time_limit')) / 0.55; //Looks like one period matches 0.55sec.
          $cid = $call->getCid();
          //#1430227 - Equivalent of the wait function?: https://www.tropo.com/account/tickets/tickets.jsp?bb-cid=155&bb-tid=1429227&bb-name=tropo_support
          for ($i = 0; $i < $time_limit; $i++) {
            $periods .= ". ";
          }
          $tropo->say($periods);

          $menu_path = 'voip/tropo/callhandler';
          $response_url = $menu_path . '/process_wait/' . $cid;
          $redirect_url = url($response_url, array('absolute' => TRUE));
          $tropo->on(array("event" => "continue", "next" => $redirect_url));

          $response_url = $menu_path . '/process_hangup/' . $cid;
          $action = url($response_url, array('absolute' => TRUE));
          $tropo->on(array("event" => "hangup", "next" => $action));
          $processing = FALSE;
          break;

      }
      // go to next command
    }
  }

  // save script for next iteration
  $call->setScript($script);
  $call->save();
  return $rc;
}

function _voiptropo_file_save_upload() {
  $upload_dir_stream = file_default_scheme() . '://';
  $_FILES['files'] = $_FILES['filename'];
  foreach ($_FILES['files'] as $key => $value) {
    $_FILES['files'][$key] = array(0 => $value);
  }
  $validators['file_validate_extensions'] = array('mp3 wav');
  $file = file_save_upload(0, $validators, $upload_dir_stream);
  if (!$file) {
    watchdog('voiptropo', "ERROR - file_save_upload failed", array(), WATCHDOG_ERROR);
    return NULL;
  }
  return $file;
}

/**
 * Update 'call status' and 'hangup reason' based on tropo's response
 */
function voiptropo_update_call_status($call, $result, $call_status) {
//TODO: should call status be updated based on $result->getState()?
  $found = TRUE;
  $state = $result->getState();
  switch ($state) {
    default:
      $msg = 'Invalid call state: @state for call cid: @cid. Request result: @result';
      $cid = $call->getCid();
      $params = array(
        '@state' => $state,
        '@cid' => $cid,
        '@result' => print_r($result, TRUE)
      );
      // $call->report_error('voiptropo', $msg, $params);
      watchdog('voiptropo', $msg, $params, WATCHDOG_WARNING);
      $found = FALSE;
      break;
    case 'RINGING':
      $call->setCallStatus(VoipCall::RINGING);
      break;
    case 'ANSWERING':
    case 'ANSWERED':
      $call->setCallStatus(VoipCall::IN_PROGRESS);
      break;
    case 'REJECTING':
    case 'REJECTED':
      $call->setCallStatus(VoipCall::REJECTED);
      break;
    case 'DISCONNECTED':
      // For Tropo, this state does not necessarily mean that the call has
      // completed sucessfully. Even calls with invalid numbers return
      // 'DISCONNECTED'
      if ($call_status == 'incomplete') {
        $call->setCallStatus(VoipCall::NO_ANSWER);
      }
      else {
        $call->setCallStatus(VoipCall::COMPLETED);
      }
      break;
    case 'FAILED':
      $msg = 'Tropo server error for call cid: @cid. Request result: @result';
      $cid = $call->getCid();
      $params = array('@cid' => $cid, '@result' => print_r($result, TRUE));
      $call->report_error('voiptropo', $msg, $params);
      break;
    case 'REDIRECTING':
    case 'REDIRECTED':
      $call->setCallStatus(VoipCall::IN_PROGRESS);
      break;
  }
  if (!$found) {
    switch ($call_status) {
      default:
        $msg = 'Invalid CallStatus: @status for call cid: @cid';
        $cid = $call->getCid();
        $params = array('@status' => $call_status, '@cid' => $cid);
        $call->report_error('voiptropo', $msg, $params);
        break;
      case 'incomplete':
        // According to https://evolution.voxeo.com/ticket/1811533, the incomplete
        // event is generated whenever there is a failure at the call level, i.e,
        // busy lines, invalid number, and no answer.
        $call->setCallStatus(VoipCall::NO_ANSWER);
        $call->setHangupReason(VoipCall::HANGUP_NO_ANSWER);
        break;
      case 'in-progress':
        $call->setCallStatus(VoipCall::IN_PROGRESS);
        break;
      case 'error':
        $msg = 'Tropo server error for call cid: @cid. Request result: @result';
        $cid = $call->getCid();
        $params = array('@cid' => $cid, '@result' => print_r($result, TRUE));
        $call->report_error('voiptropo', $msg, $params);
        break;
    }
  }
  return $call;
}


/**
 * Process the response to a "dial" script command
 */
function _voiptropo_process_dial_response($call, $request, $event_type) {
  $script = $call->getScript();
  // update the 'dial_sid' script variable
  $script->setVar('dial_sid', $request->getSessionId());

  // update the 'dial_status' script variable
  switch ($event_type) {
    default:
      $cid = $call->getCid();
      $msg = 'Invalid DialCallStatus: @status for call cid: @cid';
      $params = array('@status' => $dial_status, '@cid' => $cid);
      $call->report_error('voiptropo', $msg, $params);
      return NULL;
      break;
    case 'continue':
      $script->setVar('dial_status', VoipScript::DIAL_COMPLETED);
      break;
    case 'incomplete':
      $script->setVar('dial_status', VoipScript::DIAL_NO_ANSWER);
      break;
    case 'error':
      $script->setVar('dial_status', VoipScript::DIAL_FAILED);
      break;
  }

  $call->setScript($script);
  return $call;
}

/**
 * Process the response to a 'sms send' script command
 */
function _voiptropo_process_sms_response($call, $sms_status_tmp) {
  $cid = $call->getCid();

  switch ($sms_status_tmp) {
    default:
      $msg = 'Invalid SmsStatus: @status for call cid: @cid';
      $params = array(
        '@status' => $sms_status_tmp,
        '@cid' => $cid,
      );
      $call->report_error('voiptropo', $msg, $params);
      return NULL;
      break;
    case 'error':
      $msg = 'SMS sending failed for call cid: @cid. Make sure your tropo number is sms-enabled.';
      $params = array('@cid' => $cid);
      watchdog('voiptropo', $msg, $params, WATCHDOG_WARNING);
      $sms_status = VoipCall::TEXT_FAILED;
      break;
    case 'incomplete':
      $msg = 'Invalid SMS parameter for call cid: @cid';
      $params = array('@cid' => $cid);
      watchdog('voiptropo', $msg, $params, WATCHDOG_WARNING);
      $sms_status = VoipCall::TEXT_INVALID;
      break;
    case 'sending':
      $sms_status = VoipCall::TEXT_SENDING;
      break;
    case 'continue':
      $sms_status = VoipCall::TEXT_SENT;
      break;
  }
  $script = $call->getScript();
  $script->setVar('send_text_status', $sms_status);
  $call->setScript($script);
  $call->setTextStatus($sms_status);
  return $call;
}

function _voiptropo_prompt_validate($script, $prompt) {
  if (is_string($prompt) && (substr($prompt, 0, 1) == '^')) {
    $prompt = $script->evalString($prompt);
  }

  if (is_string($prompt)) {
    $prompt_list[] = new VoipPrompt($prompt);
  }
  elseif (is_object($prompt)) {
    $prompt_list[] = $prompt;
  }
  else {
    $prompt_list = $prompt;
  }
  return $prompt_list;
}

function _voiptropo_get_input($type, $call, $cmd, $script, &$tropo) {
  $call->setCallStatus(VoipCall::IN_PROGRESS);
  $menu_path = 'voip/tropo/callhandler';
  $cid = $call->getCid();

  $timeout = $script->evalString($cmd->getParam('timeout', 5.0));
  $end_key = $script->evalString($cmd->getParam('end_key', ''));
  $default_language = $script->getLanguage();
  $default_voice = $script->getVoice();

  if ($type == 'voice') {
    $response_url = $menu_path . '/process_get_voice_input/' . $cid;
    $choices = $script->evalString($cmd->getParam('choices'));
    $mode = $script->evalString($cmd->getParam('mode'));

    $recognize_as_tmp = $script->evalString($cmd->getParam('recognize_as'));

    //we need to get safe voice id
    $default_voice = _voiptropo_get_tropo_voice_id($default_voice);

    $default_region = VoipTropoServer::getVoiceRegionId($default_voice, $default_language);
    $recognizer = $recognize_as_tmp ? $recognize_as_tmp : $default_region;

    $allowed_modes = array('keypad', 'speech', 'any');
    if (in_array($mode, $allowed_modes)) {
      //Convert keypad to dtmf to match Tropo allowed mode values
      if ($mode == 'keypad') {
        $mode = 'dtmf';
      }
    }
    else {
      $mode = 'speech';
    }
  }
  else {
    $response_url = $menu_path . '/process_get_input/' . $cid;
    $mode = 'dtmf';
    $num_digits = $script->evalString($cmd->getParam('num_digits'), 1);
    if (empty($num_digits)) {
      //If empty it means that number of digits is variable:
      $num_digits = 999;
    }
    $grxml_path = "voip/tropo/grammar/numeric_star_hash/" . $num_digits;
    $choices = url($grxml_path, array('absolute' => TRUE));
    $recognizer = NULL;
  }
  $attempts = 1;
  $timeout = (float) $timeout;
  $prompt = _voiptropo_prompt_validate($script, $cmd->getParam('prompt'));
  $ask_text = '<speak>';
  foreach ($prompt as $p) {
    $text = $script->evalString($p->getText());
    $text = _voiptropo_strip_special_chars($text);
    $voice_id = $p->getVoice() ? $p->getVoice() : $default_voice;
    //we need to get safe voice id
    $voice_id = _voiptropo_get_tropo_voice_id($voice_id);
    $as = $p->getAs() ? $script->evalString($p->getAs()) : '';
    if (empty($as) && !in_array($as, array('digits', 'date'))) {
      $as = 'number';
    }


    //Tropo doesn't support multiple voices in ask(): https://www.tropo.com/bizblog/viewer?&bb-name=tropo_support&bb-q=&&bb-cid=99&bb-tid=1427328#bb
    //Therefore using SSML directly
    $text = _voiptropo_parse_audio($text);
    $ask_text .= '<voice name=\'' . $voice_id . '\'><say-as interpret-as=\'vxml:' . $as . '\'>' . $text . '</say-as></voice>';
  }
  $ask_text .= '</speak>';
  $params = array(
    'choices' => $choices,
    'mode' => $mode,
    'terminator' => $end_key,
    'attempts' => $attempts,
    'timeout' => $timeout,
    'recognizer' => $recognizer,
  );
  $tropo->ask($ask_text, $params);
  $action = url($response_url, array('absolute' => TRUE));
  $tropo->on(array("event" => "continue", "next" => $action));

  $action = url($response_url . '/incomplete', array('absolute' => TRUE));
  $tropo->on(array("event" => "incomplete", "next" => $action));

  $response_url = $menu_path . '/process_hangup/' . $cid;
  $action = url($response_url, array('absolute' => TRUE));
  $tropo->on(array("event" => "hangup", "next" => $action));
}

function _voiptropo_parse_numbers($str) {
  $str_array = explode(" ", $str);
  foreach ($str_array as $key => $word) {
    if (!_voiptropo_is_url($word)) {
      $str_array[$key] = preg_replace_callback('{(\d+)}', '_voiptropo_ssml_digits', $word);
    }
  }

  return implode(" ", $str_array);
}

/*Replace url with <audio> tag*/
function _voiptropo_parse_audio($str) {
  $str_array = explode(" ", $str);
  foreach ($str_array as $key => $word) {
    if (_voiptropo_is_url($word)) {
      $str_array[$key] = "<audio src='" . $word . "'/>";
    }
  }

  return implode(" ", $str_array);
}

function _voiptropo_ssml_digits($matches) {
  return "<say-as interpret-as='vxml:digits'>" . $matches[0] . "</say-as>";
}

/**
 * Check if the given string is a url
 */
function _voiptropo_is_url($str) {
  // Note: Although the checking performed is not as complete as
//       http://stackoverflow.com/questions/161738/what-is-the-best-regular-expression-to-check-if-a-string-is-a-valid-url , it seems to serve our purposes!
  $rc = preg_match('#(https?://\S+(?<![,.;?\:\!]))#', $str);
  return $rc;
}

function _voiptropo_strip_special_chars($string) {
  //Tropo doesn't support double quotes https://www.tropo.com/bizblog/viewer?&bb-name=tropo_support&bb-q=quotes&&bb-cid=100&bb-tid=1337679#bb
  $string = str_replace('"', '&quot;', $string);
  //This would be better but it breaks SSML:
  //$string = htmlspecialchars($string);
  return $string;
}

function _voiptropo_handle_new_calls($tropo_action, $session) {
  $rc = FALSE;

  // Determine current channel and network
  if ($tropo_action == 'process_outbound_calls') {
    $channel = 'voice';
    $network = 'voip';
  }
  else {
    if ($tropo_action == 'process_outbound_text') {
      $channel = 'text';
      $network = 'sms';
    }
    else {
      // handle process_inbound_calls
      $tmp = $session->getTo();
      $channel = strtolower($tmp['channel']);
      $network = strtolower($tmp['network']);
    }
  }

  // Determine call origin and destination
  if (($tropo_action == 'process_outbound_calls')
    || ($tropo_action == 'process_outbound_text')
  ) {
    $from = $session->getParameters('caller_number');
    $to = $session->getParameters('dest_number');
  }
  else {
    $tmp = $session->getFrom();
    $from = $tmp['id'];
    $tmp = $session->getTo();
    $to = $tmp['id'];
  }

  //Normalize the numbers
  $from = VoipTropoServer::NormalizeNumber($from);
  $to = VoipTropoServer::NormalizeNumber($to);

  // determine the cid and voip_session
  $cid = $session->getParameters('cid');

  // load VoIP Drupal session and cid based on request parameters
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
    // load global $user based on call user (only if we have $call object)
    global $user;
    $call->loadGlobalUser();
    if (voip_debug()) {
      watchdog('voiptropo', 'Logged user %uid in.', array('%uid' => $user->uid));
    }
  }
  else {
    $call = new VoipCall();
  }

  // handle incoming call requests (both text and voice)
  if ($tropo_action == 'process_inbound_calls' ||
    $tropo_action == 'process_sms_framework'
  ) {
    $call_id = $session->getId();
    $call->setCallId($call_id);
    $call->setSource('tropo');
    $dir_tmp = VoipCall::INBOUND;
    $call->setDirection($dir_tmp);
    $call->setAnsweredBy(VoipCall::ANSWERED_DRUPAL);
    $call->setStartTime(time());
    $call->setCallerNumber($from);
    $tmp = $session->getFrom();
    if ($tmp['name']) {
      $call->setCallerName($tmp['name']);
    }
    $call->setDestNumber($to);
    $call->setCallChannel($channel);
    $call->setCallNetwork($network);
  }
  else {
    // handle outbound call requests
    // note that those calls have already been set by the server
    // store tropo call id for all outbound calls
    $call_id = $session->getId();
    $call->setCallId($call_id);
  }

  // update common call variables
  //$call->setCurrentChannel($channel);
  //$call->setCurrentNetwork($network);
  // set text channel variables
  if (strtolower($channel) == 'text') {
    $call->SetCallStatus(VoipCall::IN_PROGRESS);
    $inbound_text_contents = $session->getInitialText();
    //Must save call before calling setTextVariables()
    $call = $call->save();

    if ($tropo_action == 'process_sms_framework') {
      $call->setCallOrigin('smsframework');
    }

    $call->setTextVariables($inbound_text_contents, $from, $to, $network);
  }
  else {
    $call->setCallStatus(VoipCall::RINGING);
  }
  // update the call object and, if needed, generate a new cid
  $call = $call->save();

  // update the call session object
  $cid = $call->getCid();
  if (!$voip_session->getCid()) {
    $voip_session->setCid($cid);
  }
  $voip_session->resetTimer();
  $voip_session->save();

  // process the call requests
  $tropo = new Tropo();
  $menu_path = 'voip/tropo/callhandler';

  if (($tropo_action == 'process_outbound_calls') || ($tropo_action == 'process_outbound_text')) {

    if ($tropo_action == 'process_outbound_calls') {
      //Make outbound call
      $dest_number = $call->getDestNumber();
      $params = array(
        'from' => $call->getCallerNumber(),
        'allowSignals' => 'hangup',
      );
      $tropo->call($dest_number, $params);

      //Add events for outbound dial.
      $response_url = $menu_path . '/process_hangup/' . $cid;
      $action = url($response_url, array('absolute' => TRUE));
      //TODO: Check about this
      $tropo->on(array("event" => "hangup", "next" => $action));

      $response_url = $menu_path . '/process_callstatus/' . $cid;
      $action = url($response_url . '/incomplete', array('absolute' => TRUE));
      $tropo->on(array("event" => "incomplete", "next" => $action));

      $action = url($response_url . '/in-progress', array('absolute' => TRUE));
      $tropo->on(array("event" => "continue", "next" => $action));

      $action = url($response_url . '/error', array('absolute' => TRUE));
      $tropo->on(array("event" => "error", "next" => $action));

      // $response_url = $menu_path . '/process_recorded_test/' . $cid;
      //$action = url($response_url, array('absolute' => TRUE));
      /* $record_params = array(
         'say' =>'start',
         'beep' => 'false',
        //'timeout' => '10.0',
      'maxSilence' => '1.0',
      'maxTime' => '10.0',
      //'url' => $action,
      );

      $tropo->record($record_params);

      $response_url = $menu_path . '/process_voice_analysis/' . $cid;
      $action = url($response_url, array('absolute' => TRUE));
      $tropo->on(array("event" => "continue", "next" => $action));
       */
    }
    else {
      if ($tropo_action == 'process_outbound_text') {
        //When using send_text then $to is already filled from dest_number param
        if (!$to) {
          $to = $call->getDestNumber();
        }

        $text = $session->getParameters('text');
        $params = array(
          'to' => $to,
          'channel' => 'TEXT',
          'network' => 'SMS',
        );
        $tropo->message($text, $params);

        //Register events:
        $menu_path = 'voip/tropo/callhandler';
        $cid = $call->getCid();

        $callback_url = $menu_path . '/process_send_sms/' . $cid;
        $callback_continue = url($callback_url . '/continue', array('absolute' => TRUE));

        $tropo->on(array("event" => "continue", "next" => $callback_continue));

        $callback_error = url($callback_url . '/error', array('absolute' => TRUE));
        $tropo->on(array("event" => "error", "next" => $callback_error));

        $callback_incomplete = url($callback_url . '/incomplete', array('absolute' => TRUE));
        $tropo->on(array(
          "event" => "incomplete",
          "next" => $callback_incomplete
        ));

        $callback_hangup = url($callback_url . '/hangup', array('absolute' => TRUE));
        $response_url = $menu_path . '/process_hangup/' . $cid;
        $action = url($response_url, array('absolute' => TRUE));
        $tropo->on(array("event" => "hangup", "next" => $action));
      }
    }

    // send the request to Tropo
    $tropo->renderJSON();
  }
  // special processing for incoming text messages
  else {
    if (($tropo_action == 'process_inbound_calls'
      || $tropo_action == 'process_sms_framework')
      && strtolower($call->getCallChannel()) == 'text'
    ) {
      $script = $call->getScript();
      $text_handler = $script->getVar('text_handler');
      $text_handler = !empty($text_handler) ? $text_handler : variable_get('voipcall_default_text_handler', '');
      if ($text_handler) {
        $continue_processing = call_user_func_array($text_handler, array(&$call));
        if(!$continue_processing) {
          return;
        }
      }
    }
  }

  if ($tropo_action == 'process_inbound_calls' || $tropo_action == 'process_sms_framework') {
    // return inbound calls so that they can have their scripts processed
    $rc = $call;
  }

  return $rc;
}

function _voiptropo_handle_ongoing_calls($result, $tropo_action, $cid, $event_type = '') {

  // 'process_hangup' for incoming calls does not come with a $cid
  if ($tropo_action == 'process_hangup') {
    if (!$cid) {
      // determine the cid based on the Call Id (Tropo session id)
      $call_id = $result->getSessionId();
      $cid = VoipCall::getCidFromCallId($call_id);
      if (!$cid) {
        $params = array(
          '@action' => $tropo_action,
          '@cid' => $cid,
          '@type' => $event_type,
          '@result' => print_r($result, TRUE)
        );
        watchdog('voiptropo', 'error in _voiptropo_handle_ongoing_calls for action: @action, cid: @cid, event type: @type, and result: @result', $params, WATCHDOG_ERROR);
        return FALSE;
      }
    }
  }

  // load the already existing call object
  $call = VoipCall::load($cid);

  // load global $user based on call user
  global $user;
  //$stored_user = $user;
  $call->loadGlobalUser();
  if (voip_debug()) {
    watchdog('voiptropo', 'Logged user %uid in.', array('%uid' => $user->uid));
  }
  // initialize common variables
  if ($tropo_action == 'process_sms_callback') {
    $call->SetCallStatus(VoipCall::IN_PROGRESS);
  }

  // save the call and initialize its script before setting internal variables
  $call->save();

  // update script variables

  // process responses to requests previously issued by the call script
  $is_hangup = $call->isHangup();
  if (!$is_hangup) {
    switch ($tropo_action) {
      default:
        $msg = 'Invalid tropo_action: @action';
        $params = array('action' => $tropo_action);
        $call->report_error('voiptropo', $msg, $params);
        return FALSE;
        break;
      case 'process_wait':
        // nothing special to be done here.
        break;
      case 'process_hangup':
        $call->SetCallStatus(VoipCall::COMPLETED);
        $call->setHangupReason(VoipCall::HANGUP_BY_USER);
        break;
      case 'process_conference':
        // nothing special to be done here.
        break;
      case 'process_dial':
        $call = _voiptropo_process_dial_response($call, $result, $event_type);
        break;
      case 'process_callstatus':
        $call = voiptropo_update_call_status($call, $result, $event_type);
        break;
      case 'process_get_input':
        $script = $call->getScript();
        if ($event_type == 'incomplete') {
          $digits = '';
        }
        else {
          $digits = str_replace(" ", "", $result->getValue());
        }
        $script->setVar('input_digits', $digits);
        $call->setScript($script);
        break;
      case 'process_get_voice_input':
        $script = $call->getScript();
        if ($event_type == 'incomplete') {
          $digits = '';
        }
        else {
          $digits = str_replace(" ", "", $result->getValue());
        }
        $script->setVar('input_digits', $digits);
        $call->setScript($script);
        break;
      case 'process_record':
      case 'process_record_hangup':
        //Save record duration
        $rec_actions = $result->getActions();
        $script = $call->getScript();
        if ($rec_actions->duration) {
          $duration = (int) ($rec_actions->duration / 1000);
          $script->setVar('recording_duration', $duration);
        }
        else {
          //Unfortunately, Tropo does not return the audio duration when the
          //recording is hang up in the middle. Audio file duration need
          //to be calculated directly using some of the algorithms found on the web.
          $rec_fid = $script->getVar('recording_fid');
          $file = file_load($rec_fid);
          if ($file) {
            $duration = voipcall_get_audio_duration($file);
            $script->setVar('recording_duration', $duration);
          }
        }
        if ($tropo_action == 'process_record_hangup') {
          $digits = 'hangup';
          $call->SetCallStatus(VoipCall::COMPLETED);
          $call->setHangupReason(VoipCall::HANGUP_BY_USER);
        }
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
      case 'process_send_sms':
      case 'process_sms_callback':
        $call = _voiptropo_process_sms_response($call, $event_type);
        break;
    }
  }

  // update call duration
  // get call duration from tropo
  $duration = $result->getSessionDuration();
  if (!$duration) {
    $end_time = $call->setEndTime();
    $start_time = $call->getStartTime();
    if ($start_time) {
      $duration = $end_time - $start_time;
    }
  }
  $call->setDuration($duration);

  // update the voipcall with its new values
  $call = $call->save();

  // update the call session object
  // load VoIP Drupal session and cid based on request parameters
  $voip_session = VoipSession::getSessionByCid($cid);

  if ($voip_session) {
    $voip_session->resetTimer();
  }
  else {
    $voip_session = new VoipSession();
    $voip_session->setCid($cid);
    $voip_session->setOriginNumber($call->getCallerNumber());
    $voip_session->setDestinationNumber($call->getDestNumber());
  }
  $voip_session->save();

  return $call;
}

//Get safe voice Tropo
function _voiptropo_get_tropo_voice_id($voice_id) {
  //we need to get safe voice id
  $server = VoipServer::getServer('tropo');
  $tropo_voices = $server->_get_available_voices();
  if (!isset($tropo_voices[$voice_id])) {
    $voice = VoipVoice::getVoice($voice_id);
    $gender = $voice->getGender();
    if ($gender == 'man') {
      $voice_id = $server->getDefaultManVoice()->getVoiceId();
    }
    else if ($gender == 'woman') {
      $voice_id = $server->getDefaultWomanVoice()->getVoiceId();
    }
    else {
      $voice_id = $server->getDefaultVoice()->getVoiceId();
    }
  }

  return $voice_id;
}
