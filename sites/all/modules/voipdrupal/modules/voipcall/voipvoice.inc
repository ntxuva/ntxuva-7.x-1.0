<?php
/**
 * @file
 * Implementation of VoIP Voice class
 */

class VoipVoice {

  /**
   * @variable $voice_id
   *   String with the type of voice to be used for the prompt.
   */
  private $voice_id;

  /**
   * @variable $gender
   *   String with the gender to be used for the prompt.  Allowed values:
   *   man, woman, neutral
   */
  private $gender;

  /**
   * @variable $language.
   *   String with the language entonation to be used for the prompt.
   */
  private $language;

  /**
   * @variable $description.
   *   String with the description of the voice.
   */
  private $description;

  /**
   * @variable $is_local
   *   Boolean indicating whether this voice is generated on the local Drupal
   *   server or by the remote VoIP server.
   */
  private $is_local;

  /**
   * Constructor
   */
  function __construct($voice_id, $gender, $language, $description, $is_local = FALSE) {
    $this->voice_id = $voice_id;
    $this->gender = $gender;
    $this->language = $language;
    $this->description = $description;
    $this->is_local = $is_local;
  }

  /**
   * Get the VoIP Voice instance associated with the given id
   *
   * @param $voice_id
   *   String representing the voice id
   *
   * @return
   *   VoipVoice instance associated with $voice_id
   */
  public static function getVoice($voice_id = NULL){
    //$voice_id = strtolower($voice_id);
    $server = VoipServer::getDefaultServer();
  	if (strtolower($voice_id) == 'man') {
      return $server->getDefaultManVoice();
    }
    else if (strtolower($voice_id) == 'woman') {
      return $server->getDefaultWomanVoice();
    }
    else if (empty($voice_id)) {
      return $server->getDefaultVoice();
    }

    $voipvoices = self::getVoices(NULL, NULL, NULL, NULL);

    $voipvoice = $voipvoices[$voice_id];
    if (empty($voipvoice)) {
      //If wrong voice_id
      watchdog('voipcall', 'VoipVoice::getVoice called with non-existing id: '. $voice_id, NULL, WATCHDOG_ERROR);
      //Load default voice instead
      $voipvoice = $server->getDefaultVoice();
    }
    return $voipvoice;
  }

  /**
   * Get the VoIP Voices that match the specified parameters
   *
   * @param $language
   *   String representing a language code ("en", "hr", etc...)
   *
   * @param $gender
   *   String representing the gender of the voice ("man", "woman", "neutral")
   *
   * @return
   *   Array of VoipVoice instances representing all available voices matching the criteria.
   */
   //todo: update docs
  public static function getVoices($language = NULL, $gender=NULL, $is_local=NULL, $voice_provider = NULL) {
    // call hook_voip_getvoices()
    $voices = module_invoke_all('voip_getvoices', $language , $gender, $is_local, $voice_provider);
    ksort($voices);
    return $voices;
  }



  function getVoiceId(){
    return $this->voice_id;
  }

  function getGender(){
    return $this->gender;
  }

  function getLanguage(){
    return $this->language;
  }

  function getDescription(){
    return $this->description;
  }

  function isLocal() {
    return $this->is_local;
  }

  /**
   * Returns numbers in digit format: '123' will become '1 2 3'
   *
   * @param $number
   *   Integer representing the number to be converted
   *
   * @return
   *   String representing the number digits
   */
  public static function getDigits($number) {
    $number_array = str_split($number);
    $number_with_space = implode(" ",$number_array);
    return $number_with_space;
  }

  /**
   * Returns time in date format
   *
   * @param $time
   *   Mixed representing time as timestamp or as any string format supported
   *   by strtotime(). Check http://php.net/manual/en/function.strtotime.php
   *
   * @return
   *   String representing the date
   */
  public static function getDate($time) {
    $timestamp = strtotime($time);  //Will convert string to timestamp
    if (!$timestamp) {
      //If the $time param was timestamp, then strtotime() will return FALSE.
      $timestamp = $time;
    }
    return date("F j, Y",$timestamp);
  }

  /**
   * Returns a voice prompt ready to be processed by a VoIP server
   *
   * This method returns the VoipPrompt instance
   *
   * @param $text
   *   String with the prompt
   *
   * @param $args
   *   An associative array of replacements to be made in the prompt afters
   *   translation. Similar to the $args parameter of t()
   *
   * @param $as
   *   String that determines how numbers should be pronounced. Supported values
   *   include 'number' (to read '12' as 'twelve') or 'digits' (to read '12' as
   *   'one two'). By default, numbers should be read as 'number'.
   *
   * @return
   *   VoipPrompt instance combining text to be read by VoIP
   *   servers.
   */
  function getPrompt($text, $args = array(), $as = NULL) {
    $prompt_text = t($text, $args, array('langcode' => $this->getLanguage()));
    return new VoipPrompt($prompt_text, $this->getVoiceId(), $as);
  }
}

/**
 * Helper functions
 */

/**
 * Stores the current voice of the system
 */
function voipvoice_set_current_voice($voice_id) {
  global $voipvoice_current_voice;
  $voipvoice_current_voice = VoipVoice::getVoice($voice_id);
}

/**
 * Wrapper around VoiceVoice's getPrompt() method
 */
function v($text, $args = array(), $as = NULL,  $voice_id=NULL) {
 if ($voice_id) {
   $voipvoice_current_voice = VoipVoice::getVoice($voice_id);
 }
 else {
   //Use voice set in voipvoice_set_current_voice()
   global $voipvoice_current_voice;
   if(!$voipvoice_current_voice) {
    //initialize with default
    $server = VoipServer::getDefaultServer();
    $voipvoice_current_voice = $server->getDefaultVoice();
   }
 }

 $prompt = $voipvoice_current_voice->getPrompt($text, $args, $as);
 return $prompt;
}

/*
* Helper function to retrieve VoipVoice instances as list array sorted by description.
*/
function voipcall_get_voices_as_list($voices) {
  $list = array();
  foreach($voices as $voice) {
    $list[$voice->getVoiceId()] = $voice->getDescription();
  }
  asort($list);
  return $list;
}

/**
 * Implements hook_voip_getvoices().
 */
function voipcall_voip_getvoices($language , $gender, $is_local, $voice_provider){
  if($voice_provider) {
    //This module is not voice provider
    return array();
  }
  if($gender == 'man') {
    $voipvoices['man'] = new VoipVoice('man', 'man', 'en', 'default man voice');
  }
  else if($gender == 'woman') {
    $voipvoices['woman'] = new VoipVoice('woman', 'woman', 'en', 'default woman voice');
  }
  else {
    $voipvoices['man'] = new VoipVoice('man', 'man', 'en', 'default man voice');
    $voipvoices['woman'] = new VoipVoice('woman', 'woman', 'en', 'default woman voice');
  }

  return $voipvoices;
}
