<?php

// $Id$

/**
 * @file
 * Implementation of the VoipPlivoCloudServer class
 */


/**
 * File dependencies
 */

module_load_include('inc', 'voip', 'includes/voipserver');

/**
 * Class definitions
 */

class VoipPlivoCloudServer extends VoipServer {

  /*
   * Constructors
   */

  function __construct() {
    $id = 'plivocloud';
    $name = 'Plivo Cloud';
    $config_form = 'voipplivocloud_admin_form';
    parent::__construct($id, $name, $config_form);
  }


  /*
   * Public methods
   */
  /**
   * Returns the Tropo Voice Region id associated with the given $voice and $language
   */
  static function getLangRegionId($voice_id) {
    $voices = VoipPlivoCloudServer::_get_available_voices();
    if ($voices[$voice_id]['region']) {
      $voice_region = $voices[$voice_id]['language'] . '-' . $voices[$voice_id]['region'];
    }
    else{
      //some languages like arabic doesn't have region
      $voice_region = $voices[$voice_id]['language'];
    }
    return $voice_region;
  }

  /**
   * Implementation of VoipServer() API method
   */

  /*
   * API methods
   */

  /**
   * Implementation of the VoipServer dial() API method
   */
  function dial($voipcall, $server_config = NULL) {

    $rc = FALSE;
    voip_error_reset();

    if (!$server_config) {
      $server_config = $this->getConfig();
    }

    // Plivo configuration
    $auth_id = $server_config['voipplivocloud_auth_id'];
    $auth_token = $server_config['voipplivocloud_auth_token'];

	  $caller_number = ($voipcall->getCallerNumber())
      ? $voipcall->getCallerNumber()
      : variable_get('voipcall_cid_number', NULL);
    $dest_number = $voipcall->getDestNumber();

   // (re)start the session associated with the call
    $voip_session = VoipSession::getSessionByNumber($caller_number, $dest_number);
    if ($voip_session) {
      $voip_session->resetTimer();
	   //we must preserve cid
	  $cid = $voip_session->getCid();
	  //load existing call
	  $voipcall = VoipCall::load($cid);
    }
    else {
      $voip_session = new VoipSession();
      $voip_session->setOriginNumber($caller_number);
      $voip_session->setDestinationNumber($dest_number);

	  // Update call variables
      $voipcall->setSource('drupal');
      $voipcall->setDirection(VoipCall::OUTBOUND);
	    $voipcall->save();
      $cid = $voipcall->getCid();
	    $voip_session->setCid($cid);
    }
    $voip_session->save();

    $menu_path = 'voip/plivocloud/callhandler';
    $response_url = $menu_path . '/process_outbound_calls/' . $cid;
    $answer_url = url($response_url, array('absolute' => TRUE));
    $response_url = $menu_path . '/process_hangup/' . $cid;
    $hangup_url = url($response_url, array('absolute' => TRUE));

    $params = array(
      'to' => $dest_number,
      'from' => $caller_number,
      'answer_url' => $answer_url,
      'hangup_url' => $hangup_url,
      'machine_detection' => 'true', //Used to detect if the call has been answered by a machine.
    );

    // Instantiate a new Plivo Client
    try{
      $client = new RestAPI($auth_id, $auth_token);
      $response = $client->make_call($params);
      if(isset($response['response']['error'])){
        switch($response['status']) {
          case '400':
          case '401':
            $error_msg = $response['response']['error'];
            $voipcall->setErrorMessage($error_msg);
            $voipcall->setCallStatus(VoipCall::ERROR);
            $voipcall->save();
            $rc = FALSE;
            break;
          default:
            // processing error
            $msg = "Processing failure. Please contact your system's administrator. Plivo error @plivo_code: @plivo_msg";
            $params = array(
              '@plivo_code' => $response['status'],
              '@plivo_msg' => $response['response']['error'],
            );
            $voipcall->report_error('voipplivocloud', $msg, $params);
            $rc = FALSE;
        }
      }
      else {
        $rc = TRUE;
      }
    }
    catch (Exception $e) {
      // wrong auth token and auth sid or if you your ip is not allowed in ALLOWED_IPS
      $error_msg = t('Wrong auth token and auth sid or ip is not allowed.');
      $voipcall->setErrorMessage($error_msg);
      $voipcall->setCallStatus(VoipCall::ERROR);
      $voipcall->save();
      $rc = FALSE;
    }


    return $rc;
  }

  /**
   * Implementation of the VoipServer send_text() API method
   */
  function send_text($text, $voipcall, $reply = FALSE, $server_config = NULL) {
    $rc = FALSE;
    voip_error_reset();

    // determine from/to numbers
    if ($reply) {
      $to = $voipcall->getCallerNumber();
      $from = ($voipcall->getDestNumber())
        ? $voipcall->getDestNumber()
        : variable_get('voipcall_cid_number', NULL);
      if (!isset($from)) {
        $voipcall->report_error('voipplivocloud', "Attempt to send a text message reply with no caller number.");
        return FALSE;
      }
    }
    else {
      $to = $voipcall->getDestNumber();
      $from = ($voipcall->getCallerNumber())
        ? $voipcall->getCallerNumber()
        : variable_get('voipcall_cid_number', NULL);
    }

    // (re)start the session associated with the call
    $voip_session = VoipSession::getSessionByNumber($from, $to);
    if ($voip_session) {
      $voip_session->resetTimer();
	    $cid = $voip_session->getCid();
	    //load existing call
	    $voipcall = VoipCall::load($cid);
    }
    else {
      $voip_session = new VoipSession();
      $voip_session->setOriginNumber($from);
      $voip_session->setDestinationNumber($to);

	  // update call variables
      $voipcall->setSource('drupal');
      if(!$voipcall->getDirection()) {
        $voipcall->setDirection(VoipCall::OUTBOUND);
      }
      if(!$voipcall->getCallChannel()) {
        $voipcall->setCallChannel('text');
      }

      if (!$voipcall->getCallNetwork()) {
        $voipcall->setCallNetwork('sms');
      }
	    $voipcall->save();
      $cid = $voipcall->getCid();
	    $voip_session->setCid($cid);
    }

    $voip_session->save();

    // send the sms
    if (voipscript_use_sms_framework()) {
      // use SMS Framework to send SMS message if user enabled this option
      voipscript_send_sms_framework($to, $text);
      return TRUE;
    }
    // else use VoIP Drupal:
    // Plivo configuration
    if (!$server_config) {
      $server_config = $this->getConfig();
    }
    $auth_id = $server_config['voipplivocloud_auth_id'];
    $auth_token = $server_config['voipplivocloud_auth_token'];

    try {
      // Instantiate a new Plivo Rest Client
      $client  = new RestAPI($auth_id, $auth_token);
      $params = array(
        'src' => $from,
        'dst' => $to,
        'text' => $text,
        'type' => 'sms',
      );
      $response = $client->send_message($params);

      if(isset($response['response']['error'])){
        switch($response['status']) {
          case '400':
          case '401':
            $error_msg = $response['response']['error'];
            $voipcall->setErrorMessage($error_msg);
            $voipcall->setCallStatus(VoipCall::ERROR);
            $voipcall->save();
            $rc = FALSE;
            break;
          default:
            // processing error
            $msg = "Processing failure. Please contact your system's administrator. Plivo error @plivo_code: @plivo_msg";
            $params = array(
              '@plivo_code' => $response['status'],
              '@plivo_msg' => $response['response']['error'],
            );
            $voipcall->report_error('voipplivocloud', $msg, $params);
            $rc = FALSE;
        }
      }
      else {
        $rc = TRUE;
      }
    }
    catch (Exception $e) {
      // wrong auth token and auth sid or if you your ip is not allowed in ALLOWED_IPS
      $error_msg = t('Wrong auth token and auth sid or ip is not allowed.');
      $voipcall->setErrorMessage($error_msg);
      $voipcall->setCallStatus(VoipCall::ERROR);
      $voipcall->save();
      $rc = FALSE;
    }

    return FALSE;
  }

  /**
   * Implementation of the VoipServer hangup() API method
   */
  function hangup($voipcall, $server_config = NULL) {
    if (voip_debug()) {
      $id = $voipcall->getCallId();
      $status = $voipcall->getCallStatus();
      watchdog('voipplivocloud', "in hangup() with call id: $id and status: $status");
    }
    $rc = FALSE;
    voip_error_reset();

    if (!$server_config) {
      $server_config = $this->getConfig();
    }

    // Plivo configuration
    $auth_id = $server_config['voipplivocloud_auth_id'];
    $auth_token = $server_config['voipplivocloud_auth_token'];

    $call_id = $voipcall->getCallId();
    if (!$call_id) {
      $msg = "Attempt to hangup a Plivo call (cid = @cid) without a call id";
      $params = array('@cid' => $voipcall->getCid());
      $voipcall->report_error('voipplivocloud', $msg, $params);
      $rc = FALSE;
    }
    else {
      $params = array(
        'call_uuid' => $call_id,
      );
       try {
         // Instantiate a new Plivo Rest Client
         $client = new RestAPI($auth_id, $auth_token);
         $response = $client->hangup_call($params);

         // update call based on response
         if (isset($response['response']['error'])) {
           $msg = "Plivo hangup command failed: @error";
           $params = array('@error' => $response['response']['error']);
           $voipcall->report_error('voipplivocloud', $msg, $params);
         }
         else {
           $voipcall->setHangupReason(VoipCall::HANGUP_API_REQUEST);
           $voipcall->save();
           $rc = TRUE;
         }
       }
       catch (Exception $e) {
         // wrong auth token and auth sid or if you your ip is not allowed in ALLOWED_IPS
         $error_msg = t('Wrong auth token and auth sid or ip is not allowed.');
         $msg = "Plivo hangup command failed: @error";
         $params = array('@error' => $error_msg);
         $voipcall->report_error('voipplivocloud', $msg, $params);
         $rc = FALSE;
       }

    }

    return $rc;
  }

  /**
   * Implementation of the VoipServer text_handler() API method
   */
  function text_handler($text_content, $origin, $destination, $time, $network, $server_name) {
    $menu_path = 'voip/plivocloud/callhandler';
    $request_url = $menu_path . '/process_sms_framework/';
    $url = url($request_url, array('absolute' => TRUE));

    $length = 20;
    $characters = '0123456789abcdefghijklmnopqrstuvwxyz';
    $characters_len = strlen($characters) - 1;
    $random_string = '';
    for ($p = 0; $p < $length; $p++) {
      $random_string .= $characters[mt_rand(0, $characters_len)];
    }

    $server_config = $this->getConfig();
    $vars = array(
      'AccountSid' => $server_config['voipplivocloud_auth_id'],
      'SmsSid' => $random_string,
      'From' => $origin,
      'To' => $destination,
      'Body' => $text_content,
      'Direction' => 'inbound',
      'Channel' => 'text'
    );

    //url-ify the data for the POST
    foreach ($vars as $key => $value) {
      $fields_string .= $key . '=' . urlencode($value) . '&';
    }
    $fields_string = rtrim($fields_string, '&');

    //open connection
    $ch = curl_init();

    //set the url, number of POST vars, POST data
    curl_setopt($ch, CURLOPT_URL, $url);
    curl_setopt($ch, CURLOPT_POST, count($vars));
    curl_setopt($ch, CURLOPT_POSTFIELDS, $fields_string);
    curl_setopt($ch, CURLOPT_RETURNTRANSFER, TRUE);

    //execute post
    $result = curl_exec($ch);

    //close connection
    curl_close($ch);
  }

  /**
   * Implementation of the VoipServer ping() API method
   */
  function ping($server_config = NULL) {

    if (!$server_config) {
      $server_config = $this->getConfig();
    }

    // Plivo configuration
    $auth_id = $server_config['voipplivocloud_auth_id'];
    $auth_token = $server_config['voipplivocloud_auth_token'];

    // Instantiate a new Plivo Rest Client
    try {
	  $client = new RestAPI($auth_id, $auth_token);

      //Request a account information as a way to test the server connectivity
      voip_error_reset();
      $vars = array();
      $response = $client->get_account($vars);

      if (!$response['response']) {
        voip_error("Plivo Cloud command Ping failed: " . $response['status']);
        return FALSE;
      }
      return TRUE;
    } catch (Exception $e) {
      voip_error($e->getMessage());
      return FALSE;
    }
  }

  /**
   * Private methods
   */
  static function _get_available_voices() {
    $voices = array();
    $voices['plivo-arabic'] = array(
      'language' => 'arabic', 'voice' => 'man',
      'description' => 'plivo-arabic (' . t('Male voice in Arabic') . ')'
    );

	$voices['plivo-chinese-woman'] = array(
      'language' => 'zh', 'region' => 'CN', 'voice' => 'woman',
      'description' => 'plivo-chinese-woman (' . t('Female voice in Chinese'). ')'
    );
	$voices['plivo-chinese-man'] = array(
      'language' => 'zh', 'region' => 'CN', 'voice' => 'man',
      'description' => 'plivo-chinese-man (' . t('Male voice in Chinese'). ')'
    );

	$voices['plivo-czech-woman'] = array(
      'language' => 'cs', 'region' => 'CZ', 'voice' => 'woman',
      'description' => 'plivo-czech-woman (' . t('Female voice in Czech'). ')'
    );

	$voices['plivo-danish-woman'] = array(
      'language' => 'da', 'region' => 'dk', 'voice' => 'woman',
      'description' => 'plivo-danish-woman (' . t('Female voice in Danish'). ')'
    );

	$voices['plivo-german-woman'] = array(
      'language' => 'de', 'region' => 'DE', 'voice' => 'woman',
      'description' => 'plivo-german-woman (' . t('Female voice in German'). ')'
    );
	$voices['plivo-german-man'] = array(
      'language' => 'de', 'region' => 'DE', 'voice' => 'man',
      'description' => 'plivo-german-man (' . t('Male voice in German'). ')'
    );

	$voices['plivo-greek-woman'] = array(
      'language' => 'el', 'region' => 'GR', 'voice' => 'woman',
      'description' => 'plivo-greek-woman (' . t('Female voice in Greek'). ')'
    );

	$voices['plivo-australian-woman'] = array(
      'language' => 'en', 'region' => 'AU', 'voice' => 'woman',
      'description' => 'plivo-australian-woman (' . t('Female voice in English (Australia)'). ')'
    );

    $voices['plivo-canada-woman'] = array(
      'language' => 'en', 'region' => 'CA', 'voice' => 'woman',
      'description' => 'plivo-canada-woman (' . t('Female voice in English (Canada)'). ')'
    );

    $voices['plivo-uk-woman'] = array(
      'language' => 'en', 'region' => 'GB', 'voice' => 'woman',
      'description' => 'plivo-uk-woman (' . t('Female voice in English (UK)'). ')'
    );
	$voices['plivo-uk-man'] = array(
      'language' => 'en', 'region' => 'GB', 'voice' => 'man',
      'description' => 'plivo-uk-man (' . t('Male voice in English (UK)'). ')'
    );

	$voices['plivo-us-woman'] = array(
      'language' => 'en', 'region' => 'US', 'voice' => 'woman',
      'description' => 'plivo-us-woman (' . t('Female voice in English (US)'). ')'
    );
	$voices['plivo-us-man'] = array(
      'language' => 'en', 'region' => 'US', 'voice' => 'man',
      'description' => 'plivo-us-man (' . t('Male voice in English (US)'). ')'
    );

	$voices['plivo-spanish-woman'] = array(
      'language' => 'es', 'region' => 'ES', 'voice' => 'woman',
      'description' => 'plivo-spanish-woman (' . t('Female voice in Spanish'). ')'
    );
	$voices['plivo-spanish-man'] = array(
      'language' => 'es', 'region' => 'ES', 'voice' => 'man',
      'description' => 'plivo-spanish-man (' . t('Male voice in Spanish'). ')'
    );

	$voices['plivo-usspanish-woman'] = array(
      'language' => 'es', 'region' => 'US', 'voice' => 'woman',
      'description' => 'plivo-usspanish-woman (' . t('Female voice in Spanish (US)'). ')'
    );
	$voices['plivo-usspanish-man'] = array(
      'language' => 'es', 'region' => 'US', 'voice' => 'man',
      'description' => 'plivo-usspanish-man (' . t('Male voice in Spanish (US)'). ')'
    );

	$voices['plivo-finland-woman'] = array(
      'language' => 'fi', 'region' => 'FI', 'voice' => 'woman',
      'description' => 'plivo-finland-woman (' . t('Female voice in Finnish'). ')'
    );

	$voices['plivo-franceca-woman'] = array(
      'language' => 'fr', 'region' => 'CA', 'voice' => 'woman',
      'description' => 'plivo-franceca-woman (' . t('Female voice in French (Canada)'). ')'
    );
	$voices['plivo-franceca-man'] = array(
      'language' => 'fr', 'region' => 'CA', 'voice' => 'man',
      'description' => 'plivo-franceca-man (' . t('Male voice in French (Canada)'). ')'
    );

	$voices['plivo-france-woman'] = array(
      'language' => 'fr', 'region' => 'FR', 'voice' => 'woman',
      'description' => 'plivo-france-woman (' . t('Female voice in French'). ')'
    );
	$voices['plivo-france-man'] = array(
      'language' => 'fr', 'region' => 'FR', 'voice' => 'man',
      'description' => 'plivo-france-man (' . t('Male voice in French'). ')'
    );

	$voices['plivo-hungary-woman'] = array(
      'language' => 'hu', 'region' => 'HU', 'voice' => 'woman',
      'description' => 'plivo-hungary-woman (' . t('Female voice in Hungarian'). ')'
    );

	$voices['plivo-italy-woman'] = array(
      'language' => 'it', 'region' => 'IT', 'voice' => 'woman',
      'description' => 'plivo-italy-woman (' . t('Female voice in Italian'). ')'
    );
	$voices['plivo-italy-man'] = array(
      'language' => 'it', 'region' => 'IT', 'voice' => 'man',
      'description' => 'plivo-italy-man (' . t('Male voice in Italian'). ')'
    );

	//bug in Plivo cloud it should be ja-JP instead of jp-JP
	$voices['plivo-japan-woman'] = array(
      'language' => 'ja', 'region' => 'JP', 'voice' => 'woman',
      'description' => 'plivo-japan-woman (' . t('Female voice in Japanese'). ')'
    );
	$voices['plivo-japan-man'] = array(
      'language' => 'jp', 'region' => 'JP', 'voice' => 'man',
      'description' => 'plivo-japan-man (' . t('Male voice in Japanese')
    );

	$voices['plivo-dutch-woman'] = array(
      'language' => 'nl', 'region' => 'NL', 'voice' => 'woman',
      'description' => 'plivo-dutch-woman (' . t('Female voice in Dutch'). ')'
    );
	$voices['plivo-dutch-man'] = array(
      'language' => 'nl', 'region' => 'NL', 'voice' => 'man',
      'description' => 'plivo-dutch-man (' . t('Male voice in Dutch'). ')'
    );

	$voices['plivo-poland-woman'] = array(
      'language' => 'pl', 'region' => 'PL', 'voice' => 'woman',
      'description' => 'plivo-poland-woman (' . t('Female voice in Polish'). ')'
    );

	$voices['plivo-brazil-woman'] = array(
      'language' => 'pt', 'region' => 'BR', 'voice' => 'woman',
      'description' => 'plivo-brazil-woman (' . t('Female voice in Portuguese (Brazil)'). ')'
    );

	$voices['plivo-portugal-woman'] = array(
      'language' => 'pt', 'region' => 'PT', 'voice' => 'woman',
      'description' => 'plivo-portugal-woman (' . t('Female voice in Portuguese (Portugal)'). ')'
    );

	$voices['plivo-russian-woman'] = array(
      'language' => 'ru', 'region' => 'RU', 'voice' => 'woman',
      'description' => 'plivo-russian-woman (' . t('Female voice in Russian'). ')'
    );
	$voices['plivo-russian-man'] = array(
      'language' => 'ru', 'region' => 'RU', 'voice' => 'man',
      'description' => 'plivo-russian-man (' . t('Male voice in Russian'). ')'
    );

	$voices['plivo-sweden-woman'] = array(
      'language' => 'sv', 'region' => 'SE', 'voice' => 'woman',
      'description' => 'plivo-sweden-woman (' . t('Female voice in Swedish'). ')'
    );

    return $voices;
  }

  /*
  * Plivo Cloud implementation of NormalizeNumber
  * Tropo doesn't return + sign so we need to add it
  *
  */
  static function NormalizeNumber($number){
    return VoipCall::NormalizeNumber($number);
  }
}
