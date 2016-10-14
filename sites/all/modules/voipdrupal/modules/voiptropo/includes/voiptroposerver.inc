<?php

// $Id$

/**
 * @file
 * Implementation of the VoiptropoServer class
 */


/**
 * File dependencies
 */

/*module_load_include('inc', 'voip', 'includes/voipserver');
module_load_include('inc', 'voipcall', 'voipcall');
*/

/**
 * Class definitions
 */

class VoipTropoServer extends VoipServer {

  /*
   * Constructors
   */

  function __construct() {
    $id = 'tropo';
    $name = 'Tropo';
    $config_form = 'voiptropo_admin_form';
    parent::__construct($id, $name, $config_form);
  }


  /*
   * Public methods
   */

  /**
   * Returns the Tropo Voice Region id associated with the given $voice and $language
   */
  static function getVoiceRegionId($voice, $language) {
    $voices = VoipTropoServer::_get_available_voices();
    foreach ($voices as $key => $voice_array) {
      if ($voice_array['language'] == $language && $voice_array['voice'] == $voice) {
        return $voice_array['language'] . '-' . $voice_array['region'];
      }
    }
    return NULL;
  }

  /*
   * API methods
   */

  /**
   * Implementation of the VoipServer dial() API method
   */
  function dial($voipcall, $server_config = NULL) {
    voip_error_reset();

    if (!$server_config) {
      $server_config = $this->getConfig();
    }

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

	    // update call variables
	    $voipcall->setCallerNumber($caller_number);
      $voipcall->setSource('drupal');
      $voipcall->setDirection(VoipCall::OUTBOUND);
      $voipcall->setCallChannel('voice');
      $voipcall->setCallNetwork('voip');
	    $voipcall->save();
      $cid = $voipcall->getCid();
	    $voip_session->setCid($cid);
    }
    $voip_session->save();

	 // tropo configuration
    $username = $server_config['voiptropo_account_username'];
    $password = $server_config['voiptropo_account_password'];
    $token = $server_config['voiptropo_voice_token'];

    // Instantiate a new tropo Rest Client
    $client = new SessionAPI();

    $vars = array(
      'action' => 'create',
      'username' => $username,
      'password' => $password,
      'caller_number' => $caller_number,
      'dest_number' => $dest_number,
      'tropo_action' => 'process_outbound_calls',
      'cid' => $cid,
    );

    //@todo: Handle answering machine options

    try {
      $response = $client->createSession($token, $vars);
    }
    catch (Exception $e) {
      $msg = "Processing failure. Function voip_dial() failed with the following exception: " . $e->getMessage();
      $voipcall->report_error('voiptropo', $msg);
      return FALSE;
    }
    // Else:
    return TRUE;
  }

  /**
   * Implementation of the VoipServer send_text() API method
   *
   * $text - Body of text message being sent. Should be shorter than 160 chars.
   */
  function send_text($text, $voipcall, $reply = FALSE, $server_config = NULL) {
    voip_error_reset();
    //determine from/to numbers
    if ($reply) {
      $to = $voipcall->getCallerNumber();
      $from = ($voipcall->getDestNumber()) ? $voipcall->getDestNumber() :variable_get('voipcall_cid_number', NULL);
	  if (!isset($from)) {
        $voipcall->report_error('voiptropo', "Attempt to send a text message reply with no caller number.");
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
	  //we must preserve cid
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

    if (voipscript_use_sms_framework()) {
      //use SMS Framework to send SMS message if user enabled this option
      voipscript_send_sms_framework($to, $text);
      return TRUE;
    }
    //else use VoIP Drupal:
    if (!$server_config) {
      $server_config = $this->getConfig();
    }

    // tropo configuration
    $username = $server_config['voiptropo_account_username'];
    $password = $server_config['voiptropo_account_password'];
    $token = $server_config['voiptropo_sms_token'];

    // Instantiate a new tropo Rest Client
    $client = new SessionAPI();

    $vars = array(
      'action' => 'create',
      'username' => $username,
      'password' => $password,
      'caller_number' => $from,
      'dest_number' => $to,
      'tropo_action' => 'process_outbound_text',
      'cid' => $cid,
      'text' => $text,
    );

    try {
      $response = $client->createSession($token, $vars);
    }
    catch (Exception $e) {
      $msg = "Processing failure. Function voip_text() failed with the following exception: " . $e->getMessage();
      $voipcall->report_error('voiptropo', $msg);
      return FALSE;
    }
    // Else:
    return TRUE;
  }

  /**
   * Implementation of the VoipServer hangup() API method
   */
  function hangup($voipcall, $server_config = NULL) {
    if (voip_debug()) {
      $id = $voipcall->getCallId();
      $status = $voipcall->getCallStatus();
      watchdog('voiptropo', "in hangup() with call id: $id and status: $status");
    }

    $rc = FALSE;
    voip_error_reset();

    if (!$server_config) {
      $server_config = $this->getConfig();
    }

    // tropo configuration
    $username = $server_config['voiptropo_account_username'];
    $password = $server_config['voiptropo_account_password'];
    //$token = $server_config['voiptropo_voice_token'];

    // Instantiate a new Tropo Event API
    $client = new EventAPI($username, $password);

    $call_id = $voipcall->getCallId();
    if (!$call_id) {
      $msg = "Attempt to hangup a tropo call (cid = @cid) without a call id";
      $params = array('@cid' => $voipcall->getCid());
      $voipcall->report_error('voiptropo', $msg, $params);
      $rc = FALSE;
    }
    else {
      $event = 'hangup';
      try {
        $client->sendEvent($call_id, $event);
        $voipcall->setHangupReason(VoipCall::HANGUP_API_REQUEST);
        $voipcall->save();
        $rc = TRUE;
      }
      catch (Exception $e) {
        $msg = "tropo hangup command failed: @error";
        $params = array('@error' => $e->getMessage());
        $voipcall->report_error('voiptropo', $msg, $params);
      }
    }

    return $rc;
  }

  /**
   * Implementation of the VoipServer text_handler() API method
   */
  function text_handler($text_content, $origin, $destination, $time, $network, $server_name) {
    $menu_path = 'voip/tropo/callhandler';
    //$request_url = $menu_path . '/process_inbound_calls/';
    $request_url = $menu_path . '/process_sms_framework/';
    $url = url($request_url, array('absolute' => TRUE));

    $length = 20;
    $characters = '0123456789abcdefghijklmnopqrstuvwxyz';
    $random_string = '';
    $characters_len = strlen($characters) - 1;
    for ($p = 0; $p < $length; $p++) {
      $random_string .= $characters[mt_rand(0, $characters_len)];
    }
    $vars->session->id =  $random_string;
    $vars->session->initialText =  $text_content;
    $vars->session->from->id = $origin;
    $vars->session->from->channel = 'text';
    $vars->session->from->network = 'sms';
    $vars->session->to->id = $destination;
    $vars->session->to->channel = 'text';
    $vars->session->to->network = 'sms';

    //open connection
    $ch = curl_init();

    //set the url, number of POST vars, POST data
    curl_setopt($ch, CURLOPT_URL, $url);
    $data_string = json_encode($vars);
    curl_setopt($ch, CURLOPT_CUSTOMREQUEST, "POST");
    curl_setopt($ch, CURLOPT_POSTFIELDS, $data_string);
    curl_setopt($ch, CURLOPT_RETURNTRANSFER, true);
    curl_setopt($ch, CURLOPT_HTTPHEADER, array(
        'Content-Type: application/json',
        'Content-Length: ' . strlen($data_string))
    );

    //execute post
    $result = curl_exec($ch);

    //close connection
    curl_close($ch);
  }
  /**
   * Implementation of the VoipServer ping() API method
   */
  function ping($server_config = NULL) {
    // tropo configuration
    $username = $server_config['voiptropo_account_username'];
    $password = $server_config['voiptropo_account_password'];

    voip_error_reset();
    try {
      // Instantiate a new tropo Rest Client
      $provisioning = new ProvisioningAPI($username, $password);
      // Request a list of applications as a way to test the server connectivity
      $response = $provisioning->viewApplications();
    }
    catch (Exception $e) {
      voip_error("Tropo command Ping failed: " . $e->getMessage());
      return FALSE;
    }

    return TRUE;
  }

  static function _get_available_voices() {
    $voices = array();
    $voices['carmen'] = array(
      'language' => 'es',
      'region' => 'es',
      'voice' => 'woman',
      'description' => t('Carmen (female voice in Castilian Spanish)'),
    );
    $voices['jorge'] = array(
      'language' => 'es',
      'region' => 'es',
      'voice' => 'man',
      'description' => t('Jorge (male voice in Castilian Spanish)'),
    );
    $voices['florence'] = array(
      'language' => 'fr',
      'region' => 'fr',
      'voice' => 'woman',
      'description' => t('Florence (female voice in French)'),
    );
    $voices['bernard'] = array(
      'language' => 'fr',
      'region' => 'fr',
      'voice' => 'man',
      'description' => t('Bernard (male voice in French)'),
    );
    $voices['allison'] = array(
      'language' => 'en',
      'region' => 'us',
      'voice' => 'woman',
      'description' => t('Allison (female voice in US English)'),
    );
    $voices['dave'] = array(
      'language' => 'en',
      'region' => 'us',
      'voice' => 'man',
      'description' => t('Dave (male voice in US English)'),
    );
    $voices['kate'] = array(
      'language' => 'en',
      'region' => 'gb',
      'voice' => 'woman',
      'description' => t('Kate (female voice in British English)'),
    );
    $voices['simon'] = array(
      'language' => 'en',
      'region' => 'gb',
      'voice' => 'man',
      'description' => t('Simon (male voice in British English)'),
    );
    $voices['katrin'] = array(
      'language' => 'de',
      'region' => 'de',
      'voice' => 'woman',
      'description' => t('Katrin (female voice in German)'),
    );
    $voices['stefan'] = array(
      'language' => 'de',
      'region' => 'de',
      'voice' => 'man',
      'description' => t('Stefan (male voice in German)'),
    );
    $voices['paola'] = array(
      'language' => 'it',
      'region' => 'it',
      'voice' => 'woman',
      'description' => t('Paola (female voice in Italian)'),
    );
    $voices['luca'] = array(
      'language' => 'it',
      'region' => 'it',
      'voice' => 'man',
      'description' => t('Luca (male voice in Italian)'),
    );
    $voices['saskia'] = array(
      'language' => 'nl',
      'region' => 'nl',
      'voice' => 'woman',
      'description' => t('Saskia (female voice in Dutch)'),
    );
    $voices['willem'] = array(
      'language' => 'nl',
      'region' => 'nl',
      'voice' => 'man',
      'description' => t('Willem (male voice in Dutch)'),
    );
    $voices['soledad'] = array(
      'language' => 'es',
      'region' => 'mx',
      'voice' => 'woman',
      'description' => t('Soledad (female voice in Mexican Spanish)'),
    );
    $voices['carlos'] = array(
      'language' => 'es',
      'region' => 'mx',
      'voice' => 'man',
      'description' => t('Carlos (male voice in Mexican Spanish)'),
    );
    $voices['zosia'] = array(
      'language' => 'pl',
      'region' => 'pl',
      'voice' => 'woman',
      'description' => t('Zosia (female voice in Polish)'),
    );
    $voices['krzysztof'] = array(
      'language' => 'pl',
      'region' => 'pl',
      'voice' => 'man',
      'description' => t('Krzysztof (male voice in Polish)'),
    );

    $voices['jordi'] = array(
      'language' => 'ca',
      'region' => 'es',
      'voice' => 'man',
      'description' => t('Jordi (male voice in Catalan)'),
    );
    $voices['montserrat'] = array(
      'language' => 'ca',
      'region' => 'es',
      'voice' => 'woman',
      'description' => t('Montserrat (female voice in Catalan)'),
    );

    $voices['magnus'] = array(
      'language' => 'da',
      'region' => 'dk',
      'voice' => 'man',
      'description' => t('Magnus (male voice in Danish)'),
    );
    $voices['frida'] = array(
      'language' => 'da',
      'region' => 'dk',
      'voice' => 'woman',
      'description' => t('Frida (female voice in Danish)'),
    );

    $voices['mikko'] = array(
      'language' => 'fi',
      'region' => 'fi',
      'voice' => 'man',
      'description' => t('Mikko (male voice in Finnish)'),
    );
    $voices['milla'] = array(
      'language' => 'fi',
      'region' => 'fi',
      'voice' => 'woman',
      'description' => t('Milla (female voice in Finnish)'),
    );

    $voices['carmela'] = array(
      'language' => 'gl',
      'region' => 'es',
      'voice' => 'woman',
      'description' => t('Carmela (female voice in Galacian)'),
    );

    $voices['nikos'] = array(
      'language' => 'el',
      'region' => 'gr',
      'voice' => 'man',
      'description' => t('Nikos (male voice in Greek)'),
    );
    $voices['afroditi'] = array(
      'language' => 'el',
      'region' => 'gr',
      'voice' => 'woman',
      'description' => t('Afroditi (female voice in Greek)'),
    );

    $voices['linlin'] = array(
      'language' => 'cn',
      'region' => 'cn',
      'voice' => 'woman',
      'description' => t('Linlin (female voice in Mandarin Chinese)'),
    );

    $voices['henrik'] = array(
      'language' => 'no',
      'region' => 'no',
      'voice' => 'man',
      'description' => t('Henrik (male voice in Norwegian'),
    );
    $voices['vilde'] = array(
      'language' => 'no',
      'region' => 'no',
      'voice' => 'woman',
      'description' => t('Vilde (female voice in Norwegian)'),
    );

    $voices['dmitri'] = array(
      'language' => 'ru',
      'region' => 'ru',
      'voice' => 'man',
      'description' => t('Dmitri (male voice in Russian)'),
    );
    $voices['olga'] = array(
      'language' => 'ru',
      'region' => 'ru',
      'voice' => 'woman',
      'description' => t('Olga (female voice in Russian)'),
    );

    $voices['eusebio'] = array(
      'language' => 'pt',
      'region' => 'pt',
      'voice' => 'man',
      'description' => t('Eusebio (male voice in Portuguese)'),
    );
    $voices['amalia'] = array(
      'language' => 'pt',
      'region' => 'pt',
      'voice' => 'woman',
      'description' => t('Amalia (female voice in Portuguese)'),
    );

    $voices['felipe'] = array(
      'language' => 'pt',
      'region' => 'br',
      'voice' => 'man',
      'description' => t('Felipe (male voice in Portuguese (Brazilian))'),
    );
    $voices['fernanda'] = array(
      'language' => 'pt',
      'region' => 'br',
      'voice' => 'woman',
      'description' => t('Fernanda (female voice in Portuguese (Brazilian))'),
    );

    $voices['sven'] = array(
      'language' => 'sv',
      'region' => 'se',
      'voice' => 'man',
      'description' => t('Sven (male voice in Swedish)'),
    );
    $voices['annika'] = array(
      'language' => 'sv',
      'region' => 'se',
      'voice' => 'woman',
      'description' => t('Annika (female voice in Swedish)'),
    );

    $voices['empar'] = array(
      'language' => 'va',
      'region' => 'va',
      'voice' => 'woman',
      'description' => t('Empar (female voice in Valencian)'),
    );
    return $voices;
  }

  /*
  * Tropo implementation of NormalizeNumber
  * Takes into consideration that call between two numbers in same country will use domestic format.
  * In case of SMS Tropo doesn't return + sign so we need to add it.
  */
  public static function NormalizeNumber($number){
    if(!$number) {
      //Empty number
      return $number;
    }

    if(strlen($number)<11) {
      //This is domestic number, Tropo uses it only for USA/Canada
      //so we'll explicity add country code
      $number = '+1' . $number;
    }
    //Normalize number by adding '+' prefix
    return VoipCall::NormalizeNumber($number);
  }

  /**
   * Tropo version of VoipServer::audioFileMover()
   *
   * Intended for use when a message has been recorded on the server, and needs
   * to be stored or otherwise processed locally.
   *
   * @param string $cid
   *   The cid of $call
   */
  static function audioFileMover($audio_url, $callback_function, $args = array()) {
    $original_call = $args['call'];
    $original_script = $original_call->getScript();
    $recording_fid = $original_script->getVar('recording_fid');
    $file = file_load($recording_fid);
    $callback_function($file, file_create_url($file->uri), $args);
  }
}
