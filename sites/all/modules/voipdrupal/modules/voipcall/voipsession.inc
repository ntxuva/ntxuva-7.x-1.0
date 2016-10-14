<?php

class VoipSession {

  private $session_data;

  function __construct($session_data = NULL) {
    $this->session_data = $session_data;
  }

  /**
   * Returns VoipSession object associated with the matching pair of numbers
   *
   * @param $origin_number
   *   String representing origin number.
   * @param $destination_number
   *   String representing destination number.
   *
   * @return
   *   Session object.
   */
  public static function getSessionByNumber($origin_number, $destination_number) {
	  $session_data = db_query("SELECT * FROM {voipsession} WHERE (origin_number=:origin_number AND destination_number=:destination_number) OR (origin_number=:destination_number AND destination_number=:origin_number)", array(':origin_number' => $origin_number, ':destination_number' => $destination_number))->fetchObject();

    if ($session_data && $session_data->expiring_at < time()) {
      //Delete this session, its expired
      db_query("DELETE FROM {voipsession} WHERE sid=:sid", array(':sid' => $session_data->sid));
      return FALSE;
    }

    if ($session_data) {
      $session = new self($session_data);
      return $session;
    }

    return FALSE;
  }

  /**
   * Returns Session object associated with the matching VoIP Call
   *
   * @param $cid
   *   Integer representing VoIP Cid.
   *
   * @return
   *    Session instance or FALSE.
   */

  public static function getSessionByCid($cid) {
    $result = db_query("SELECT * FROM {voipsession} WHERE call_id = :call_id", array(':call_id' => $cid));

    foreach($result as $session_data) {
      if ($session_data->expiring_at < REQUEST_TIME) {
        //Delete this session, its expired
        db_delete('voipsession')
          ->condition('sid', $session_data->sid)
          ->execute();
        return FALSE;
      }

      if ($session_data) {
        $session = new self($session_data);
        return $session;
      }
    }

    return FALSE;
  }

  /**
   * Delete all expired sessions
   *
   * @return
   *   Array of call ids of deleted sessions.
   */
  public static function deleteExpiredSessions() {
    $deleted_call_ids = array();
    $time = REQUEST_TIME;

    //Get call ids of deleted sessions
    $res = db_query("SELECT * FROM {voipsession} WHERE expiring_at < :expiring_at", array(':expiring_at' => $time));
    foreach ($res as $session) {
      $deleted_call_ids[] = $session->call_id;
    }
    //Delete the sessions
    db_delete('voipsession')
      ->condition('expiring_at', $time, '<')
      ->execute();
    return $deleted_call_ids;
  }

  public function getCid() {
    //Avoid Undefined property warning
    return isset($this->session_data->call_id) ? $this->session_data->call_id : NULL;
  }

  public function setCid($cid) {
    $this->session_data->call_id = $cid;
  }

  public function getOriginNumber() {
    return $this->session_data->origin_number;
  }

  public function setOriginNumber($origin_number) {
     $this->session_data->origin_number = $origin_number;
  }

  public function getDestinationNumber() {
    return $this->session_data->destination_number;
  }

  public function setDestinationNumber($destination_number) {
    $this->session_data->destination_number = $destination_number;
  }

  public function setExpirationTime($expiration_time = NULL) {
    if (!$expiration_time) {
      $expiration_time = _voipsession_default_expiration_time($this->session_data->call_id);
    }
    $this->session_data->expiration_time = $expiration_time;
  }

  public function resetTimer($expiration_time = NULL) {
    if (!$expiration_time) {
      $expiration_time = _voipsession_default_expiration_time($this->session_data->call_id);
    }
    $this->session_data->expiration_time = $expiration_time;
    $this->session_data->expiring_at = REQUEST_TIME + $expiration_time;
  }

  /**
   * Save the session data to the database
   *
   * @return
   *   The updated VoipSession instance, or NULL
   */
  public function save() {
    if (!isset($this->session_data->sid) || is_null($this->session_data->sid)) {
      $data = new stdClass();
      $update = NULL;
      // initialize the 'Call Id' field
      if (!isset($this->session_data->call_id)) {
        $data->call_id = NULL;
      }
      // initialize the 'Origin number' field
      if (!isset($this->session_data->origin_number)) {
        $data->origin_number = NULL;
      }
      // initialize the 'Destination number' field
      if (!isset($this->session_data->destination_number)) {
        $data->destination_number = NULL;
      }
      // initialize the 'Expiration time' field
      if (!isset($this->session_data->expiration_time)) {
        $data->expiration_time = _voipsession_default_expiration_time(isset($this->session_data->call_id) ? $this->session_data->call_id : "");
        $expiration_time = _voipsession_default_expiration_time(isset($this->session_data->call_id) ? $this->session_data->call_id : "");
      }
      else {
        $expiration_time = $this->session_data->expiration_time;
      }
      // initialize the 'Expiring at' field
      $this->session_data->expiring_at = REQUEST_TIME + $expiration_time;

    }
    else {
      // load the data
      $result = db_query("SELECT * FROM {voipsession} WHERE sid = :sid", array(':sid' => $this->session_data->sid));
      $data = NULL;
      foreach($result as $info) {
        $data = $info;
        break;
      }
      $update = 'sid';
    }

    $call_id = isset($this->session_data->call_id) ? $this->session_data->call_id : NULL;
    if (isset($call_id)) {
      $data->call_id = $call_id;
    }

    $origin_number = $this->session_data->origin_number;
    if (isset($origin_number)) {
      $data->origin_number = $origin_number;
    }

    $destination_number = $this->session_data->destination_number;
    if (isset($destination_number)) {
      $data->destination_number = $destination_number;
    }

    $expiration_time = isset($this->session_data->expiration_time) ? $this->session_data->expiration_time : NULL;
    if (isset($expiration_time)) {
      $data->expiration_time = $expiration_time;
    }

    $data->expiring_at =  $this->session_data->expiring_at;
    // save the data
    if(is_null($update)) {
      drupal_write_record('voipsession', $data);
    }
    else {
      drupal_write_record('voipsession', $data, $update);
    }

    $this->session_data = $data;
  }

  /**
   * Destroy the session data to the database
   */
  public function destroy() {
    $sid = $this->session_data->sid;
    if ($sid) {
      db_delete('voipsession')
        ->condition('sid', $sid)
        ->execute();
    }
    if (voip_debug()) {
      watchdog('voipsession', "Destroying session $sid");
    }
    $this->session_data = NULL;
  }
}

function _voipsession_default_expiration_time($cid) {
  if ($cid) {
    $call = VoipCall::load($cid);
    $session_duration = $call->getMaxSessionDuration();
  }

  return isset($session_duration) ? $session_duration : variable_get('voipcall_default_session_time', 60);
}
