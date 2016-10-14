<?php

// $Id$

/**
 * @file
 * Implementation of the VoipKooKooServer class
 */

module_load_include('inc', 'voip', 'includes/voipserver');
module_load_include('inc', 'voipcall', 'voipcall');


/**
 * Class definitions
 */

class VoipKooKooServer extends VoipServer {

  /*
   * Constructors
   */

  function __construct() {
    $id = 'kookoo';
    $name = 'KooKoo';
    $config_form = 'voipkookoo_admin_form';
    parent::__construct($id, $name, $config_form);
  }

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

    // KooKoo configuration
    $api_version = 2;
    $api_key = $server_config['voipkookoo_api_key'];

    $caller_number = ($voipcall->getCallerNumber())
      ? $voipcall->getCallerNumber()
      : variable_get('voipcall_cid_number', NULL);
    //We need to adopt number to Kookoo format
    $dest_number = VoipKooKooServer::NormalizeNumber($voipcall->getDestNumber());
    $voipcall->setDestNumber($dest_number);


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

    // Initiate a new outbound call by POST'ing to the Calls resource
    $menu_path = 'voip/kookoo/callhandler';
    $response_url = $menu_path . '/process_outbound_calls/' . $cid;
    $call_handler_url = url($response_url, array('absolute' => TRUE));
    $response_url = $menu_path . '/process_hangup/' . $cid;
    $status_callback_url = url($response_url, array('absolute' => TRUE));

    $kookoo_url = "http://www.kookoo.in/outbound/outbound.php?api_key=".$api_key
      ."&url=".$call_handler_url
      ."&outbound_version=".$api_version
      ."&caller_id=".$caller_number
      ."&callback_url=".$status_callback_url
      ."&phone_no=".$dest_number;
watchdog('debug', 'dial url '.$kookoo_url);
    //Instantiate a new KooKoo Client
    $ch = curl_init();
    curl_setopt($ch, CURLOPT_URL, $kookoo_url);
    curl_setopt($ch, CURLOPT_RETURNTRANSFER, true);
    //execute post
    $response = curl_exec($ch);
    curl_close ($ch);

    $xml = new SimpleXmlElement($response);
    watchdog('debug', "dial response ".print_r($xml,true));
    // update call based on response
    if ($xml->status == 'error') {
      $error_msg = (string) $xml->message;
      // differentiate between processing errors and higher-level errors
      $voipcall->setErrorMessage($error_msg);
      $voipcall->setCallStatus(VoipCall::ERROR);
      $rc = FALSE;
    }
    else {
      $rc = TRUE;
      // get the new id and status associated with the call
      $call_id = (string) $xml->message;
      $kookoo_call_status = (string) $xml->status;
      $voipcall->setCallId($call_id);
      $voipcall = voipkookoo_update_call_status($voipcall, $kookoo_call_status);
    }
    $voipcall->save();
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
    }
    else {
      $to = $voipcall->getDestNumber();
      //From number is required in KooKoo
      $from = ($voipcall->getCallerNumber())
        ? $voipcall->getCallerNumber()
        : variable_get('voipcall_cid_number', NULL);
    }

    if (!isset($from)) {
      $voipcall->report_error('voipkookoo', "Attempt to send a text message reply with no caller number.");
      return FALSE;
    }

    //We need to adopt number to Kookoo format
    $to = VoipKooKooServer::NormalizeNumber($to);
    $voipcall->setDestNumber($to);

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
      if (!$voipcall->getDirection()) {
        $voipcall->setDirection(VoipCall::OUTBOUND);
      }
      if (!$voipcall->getCallChannel()) {
        $voipcall->setCallChannel('text');
      }

      if (!$voipcall->getCallNetwork()) {
        $voipcall->setCallNetwork('sms');
      }
      $voipcall->save();
      $cid = $voipcall->getCid();
      $voip_session->setCid($cid);
    }
    //$voip_session->setCid($voipcall->getCid());
    $voip_session->save();

    // send the sms
    if (voipscript_use_sms_framework()) {
      //Use SMS Framework to send SMS message if user enabled this option
      voipscript_send_sms_framework($to, $text);
      return TRUE;
    }
    // else use VoIP Drupal
    if (!$server_config) {
      $server_config = $this->getConfig();
    }

    // KooKoo configuration
    $api_version = 2;
    $api_key = $server_config['voipkookoo_api_key'];

    // Initiate a new outbound text by POST'ing to the SMS/Messages resource

    $menu_path = 'voip/kookoo/callhandler';
    //$cid = $voipcall->getCid();
    $response_url = $menu_path . '/process_sms_callback/' . $cid;
    $status_callback_url = url($response_url, array('absolute' => TRUE));

    $kookoo_url = "http://www.kookoo.in/outbound/outbound_sms.php?api_key=".$api_key
      ."&outbound_version=".$api_version
      ."&caller_id=".$from //todo: check this
      ."&callback_url=".$status_callback_url
      ."&phone_no=".$to
      ."&message=".$text;

    //Instantiate a new KooKoo Client
    $ch = curl_init();
    curl_setopt($ch, CURLOPT_URL, $kookoo_url);
    curl_setopt($ch, CURLOPT_RETURNTRANSFER, true);
    //execute post
    $response = curl_exec($ch);
    curl_close ($ch);

    $xml = new SimpleXmlElement($response);
    watchdog('debug', "send text response ".print_r($xml,true));
    watchdog('debug', "url ".$kookoo_url);

    if ($xml->status == 'error') {
      $error_msg = (string) $xml->message;
      // differentiate between processing errors and higher-level errors
      $voipcall->setErrorMessage($error_msg);
      $voipcall->setCallStatus(VoipCall::ERROR);
      $voipcall->save();
      $rc = FALSE;
    }
    else {
      $rc = TRUE;
      // get the new id and status associated with the call
      $call_id = (string) $xml->message;
      $kookoo_call_status = (string) $xml->status;
      $voipcall->setCallId($call_id);
      $voipcall = voipkookoo_update_call_status($voipcall, $kookoo_call_status);
      $voipcall->save();
    }

    return $rc;
  }

  /**
   * Implementation of the VoipServer hangup() API method
   */
  //@todo: whats hangup url in kookoo??

  function hangup($voipcall, $server_config = NULL) {
    if (voip_debug()) {
      $id = $voipcall->getCallId();
      $status = $voipcall->getCallStatus();
      watchdog('voipkookoo', "in hangup() with call id: $id and status: $status");
    }

    $rc = FALSE;
    voip_error_reset();
/*
    if (!$server_config) {
      $server_config = $this->getConfig();
    }

    // KooKoo configuration
    $api_version = '2010-04-01';
    $account_sid = $server_config['voipkookoo_account_sid'];
    $auth_token = $server_config['voipkookoo_auth_token'];

    // Instantiate a new KooKoo Rest Client
    $client = new KooKooRestClient($account_sid, $auth_token);

    $call_id = $voipcall->getCallId();
    if (!$call_id) {
      $msg = "Attempt to hangup a KooKoo call (cid = @cid) without a call id";
      $params = array('@cid' => $voipcall->getCid());
      $voipcall->report_error('voipkookoo', $msg, $params);
      $rc = FALSE;
    }
    else {
      $kookoo_path = "/$api_version/Accounts/$account_sid/Calls/$call_id";
      $status = 'completed';
      $vars = array(
        'Status' => $status,
      );

      $response = $client->request($kookoo_path, "POST", $vars);

      // update call based on response
      if ($response->IsError) {
        $msg = "KooKoo hangup command failed: @error";
        $params = array('@error' => $response->ErrorMessage);
        $voipcall->report_error('voipkookoo', $msg, $params);
      }
      else {
        $voipcall->setHangupReason(VoipCall::HANGUP_API_REQUEST);
        $voipcall->save();
        $rc = TRUE;
      }
    }
*/
    return $rc;
  }

  /**
   * Implementation of the VoipServer text_handler() API method
   */
  //@todo:
  function text_handler($text_content, $origin, $destination, $time, $network, $server_name) {
    $menu_path = 'voip/kookoo/callhandler';
    //$request_url = $menu_path . '/process_inbound_text/';
    $request_url = $menu_path . '/process_sms_framework/';
    $url = url($request_url, array('absolute' => TRUE));

    $server_config = $this->getConfig();
    $vars = array(
      'AccountSid' => $server_config['voipkookoo_account_sid'],
      'SmsSid' => $todo,
      'From' => $origin,
      'To' => $destination,
      'Body' => $text_content,
      'Direction' => 'inbound',
      'Channel' => 'text',
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
    //@todo: currently no reliable method in voip kookoo to determine the validity of api key
    return TRUE;
    /*if (!$server_config) {
      $server_config = $this->getConfig();
    }

    // KooKoo configuration
    $api_version = 2;
    $api_key = $server_config['voipkookoo_api_key'];



    // Request a outbound call without phone number as a way to test the server connectivity
    voip_error_reset();
    $kookoo_url = "http://www.kookoo.in/outbound/outbound.php?api_key=".$api_key."&url=ping&phone_no=000000000";

    //Instantiate a new KooKoo Client
    $ch = curl_init();
    curl_setopt($ch, CURLOPT_URL, $kookoo_url);
    curl_setopt($ch, CURLOPT_RETURNTRANSFER, true);
    //execute post
    $response = curl_exec($ch);
    curl_close ($ch);

    $xml = new SimpleXmlElement($response);
    watchdog('debug', "ping response ".print_r($xml,true));
    var_dump($xml);die();
    // update call based on response
    if ($response->response->message == 'Authentication error') {
      voip_error("KooKoo command Ping failed: " . $response->ErrorMessage);
      return FALSE;
    }*/

    return TRUE;
  }

  /*
  * Kookoo implementation of NormalizeNumber
  * Takes into consideration that Kookoo doesn't support + sign in number. All international numbers must start with 00
  */
  public static function NormalizeNumber($number){
    if(!$number) {
      //Empty number
      return $number;
    }

    //Remove special chars from number
    $chars = array(' ', '-', '.', '(', ')', '+');
    $number = str_replace($chars, '', $number);
    //Remove 00 from begining of number
    $pattern = '/^00/';
    $number = preg_replace($pattern, '', $number);
    //Explicitly add 00 in front of number
    $number = '00'.$number;
    return $number;
  }

  static function _get_available_voices() {
    $voices = array();
    $voices['EN'] = array(
      'language' => 'en', 'voice' => 'woman',
      'description' => t('female voice in English')
    );
    //@todo: In PHP library this voices are marked as valid, but they actually don't work:
    /*$voices['HI'] = array(
      'language' => 'in', 'voice' => 'man',
      'description' => t('male voice in Hindi')
    );
    $voices['TE'] = array(
      'language' => 'in', 'voice' => 'man',
      'description' => t('male voice in Telugu')
    );
    $voices['KA'] = array(
      'language' => 'in', 'voice' => 'man',
      'description' => t('male voice in Kannada')
    );*/
    return $voices;
  }
}
