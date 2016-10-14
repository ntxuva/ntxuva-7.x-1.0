
== Introduction ==

Plivo Framework (http://plivo.com/opensource) is an open source communications framework that enables the rapid development of voice-based apps using your existing web infrastructure.


== Requirements==

In order to install the voipplivoframework.module, you will need:

1. The Plivo Framework configured and running in your server (http://docs.plivo.org/get-started/)

2. The PHP Curl extension in your system. For Debian systems, just run
  $ sudo apt-get install php5-curl
  $ sudo /etc/init.d/apache2 restart


== Installation ==

Installing voipplivoframework.module is simple.  It requires a few configuration steps on your Drupal site to let it know how to reach your Plivo server. It also requires a few settings in your Plivo configuration to make sure it knows which Drupal site to use.


Plivo configuration:

1. Go to the /pathtoplivoinstall/etc/plivo and open default.conf for editing

2. Change the following values:

  - DEFAULT_HTTP_METHOD = POST

  - DEFAULT_ANSWER_URL = http://mysite.com/voip/plivoframework/callhandler/ (for clean URLs) or http://mysite.com/?q=voip/plivoframework/callhandler/

  - EXTRA_FS_VARS = variable_duration

  - AUTH_ID = enter any value, this is your authentication id

  - AUTH_TOKEN = enter any value, this is your authentication token

3. Save the file and restart Plivo


Drupal configuration:

1. Install and enable voipplivoframework.module

2. Set Plivo Framework as the default voip server

  - Go to admin/voip/servers

  - Click on Plivo Framework "configure" link

  - Fill in the "Account SID" and "Auth Token" fields with your Plivo "AUTH_ID" and "AUTH_TOKEN" values, respectively (see "Plivo configuration" above)

  - If your Plivo is on a different server than Drupal, change the value of "Plivo REST API Url" to the new server's URL

  - Optionally, click on "Plivo Outbound Call Parameters" to set up advanced options as per your needs

  - Press "Save". That will take you back to admin/voip/servers

  - Select the 'Plivo Framework' option

  - Press the 'Set default voip server' button


3. Enable users to make outgoing (outbound) calls from your site

  - Go to admin/people/permissions

  - Find the "voip module" permissions

  - Enable the "make outbound calls" permission for the desired roles

  - Press the "save permissions" button


== Try it out ==

Now you should be able to call your VoIP Drupal site on your Plivo's default number (1000@yourserverip:5080). Enjoy!

---
This module has been originally developed by Leo Burd and Tamer Zoubi under the sponsorship of the MIT Center for Civic Media (http://civic.mit.edu) and Plivo (http://plivo.com).

