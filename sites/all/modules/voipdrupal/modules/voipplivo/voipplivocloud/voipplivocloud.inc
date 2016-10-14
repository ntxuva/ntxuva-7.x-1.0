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
 * @param string $plivo_action set to the action to be executed.
 *   It can assume any of the following values:
 *   - process_conference, to handle responses to conference call requests
 *   - process_inbound_calls, to handle incoming calls
 *   - process_inbound_text, to handle incoming text
 *   - process_outbound_calls, to handle the response to outgoing call requests
 *   - process_dial, to handle the response to the "dial" script command
 *   - process_get_input, to handle responses to "get input" script commands
 *   - process_hangup, to handle call termination
 *   - process_record, process_record_mp3 and process_empty_recording, to handle
 *     responses to 'record' script commands
 *   - process_wait, to continue after wait command
 *   - process_send_sms, to handle responses to 'send sms' script commands
 *   - process_sms_callback, to handle final responses to 'send sms'.
 *
 * @param string $cid with the cid of the call
 *
 * @return void
 */
function _voipplivocloud_call_handler($plivo_action, $cid = NULL) {
  module_load_include('inc', 'voip', 'modules/voipplivo/includes/voipplivocore');
  $server = 'plivocloud';
  return voipplivocore_call_handler($server, $plivo_action, $cid);
}

function _voipplivocloud_validate_request() {
  //todo: hardcoding this until i figure out how to validate request
  return TRUE;

  $server = VoipServer::getServer('plivocloud');
  $server_config = $server->getConfig();
  $auth_token = $server_config['voipplivocloud_auth_token'];

  if(isset($_GET)) {
    $post_params = $_GET;
  } elseif(isset($_POST)) {
    $post_params = $_POST;
  }

  unset($post_params['q']);
  $signature = $_SERVER["HTTP_X_PLIVO_SIGNATURE"];
  $uri = "http://". $_SERVER["SERVER_NAME"] . $_SERVER["REQUEST_URI"]; //also tried with PHP_SELF
  watchdog('voipplivocore', "server ".print_r($_SERVER,true));
watchdog('voipplivocore', "uri $uri ");

  if (validate_signature($uri, $post_params, $signature, $auth_token)) {
    return TRUE;
  }
  return FALSE;
}
