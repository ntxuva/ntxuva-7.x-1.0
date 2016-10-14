<?php
// $id$

/**
 * @file
 * Implementation of the VoipKooKooScript class
 */

class VoipKooKooScript extends VoipScript {

  /**
   * Constructs a new instance of the VoipKooKooScript class
   */
  function __construct($name, $variables = array()) {
    parent::__construct($name, $variables);
  }

  /**
   * Add a 'sayAs' command to the script.
   *
   * Says the given prompt the specified number of times.
   *
   * @param $prompt
   * String combining text to be converted to speech.
   * @param $format
   * String with format of text to be played. Can be number, digit, date and currency.
   * @param $voice
   *   String with the voice id english => EN,hindi => HI,kannada =>KA,Malayalam=> ML,Telugu => TE,Tamil=>TN,Gujarati=> GUJ, Benagali=>
  BN,Marathi=> MR,Oriya=> ORI
   *
   * @return
   *   The execution of the 'say' command does not return anything.
   */
  function addSayAs($prompt, $format, $voice = NULL) {
    $params['prompt'] = $prompt;
    $params['format'] = $format;
    $params['voice'] = $voice;
    $command = new VoipCommand('VoipCmdSayAs', $params);
    parent::addCommand($command);
  }
}

