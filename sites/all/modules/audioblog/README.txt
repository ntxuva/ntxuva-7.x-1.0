== Introduction ==

Audioblog allows users to access and record audio blog entries from their phone. New entries can be created via SMS, regular phone call, or directly from the web.


== Requirements ==

Audio Blog module depends on cron to properly move audio recordings from VoIP providers to the local Drupal server.


== Installation ==

1. Extract audioblog.module to your sites/all/modules directory

2. Enable the "Audio Blog" module in admin/build/modules

3. Go to admin/voip/call/settings 

4. Set "audioblog_main_menu_script" as the default inbound call script 

5. Set "audioblog_sms_handler_script" as the default inbound text script


== Usage ==

- the voice channel script presents the user with a voice menu in where they can either select to listen existing Audio Blogs or to record a new Audio Blog. The recorded audio will be saved in as new node of audioblog content type.

- the text channel script enables users to create new Audio Blog entries by sending text message "ADD" followed by space and actual Blog content.

---
The Audio Blog module has been originally developed under the sponsorship of the MIT Center for Civic Media (http://civic.mit.edu).
