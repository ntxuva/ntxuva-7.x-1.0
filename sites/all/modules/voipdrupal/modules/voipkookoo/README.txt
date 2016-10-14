== Introduction ==

The voipkookoo.module makes it possible for the VoIP Drupal platform to make and receive calls via the KooKoo Cloud Telephony service (http://www.kookoo.in/).

Please check the "Important notes" section below for specific attributes and limitations of the KooKoo server.


== Requirements ==

In order to install the voipkookoo.module, you will need:

1. A KooKoo account

2. The PHP Curl extension in your system. For Debian systems, just run
  $ sudo apt-get install php5-curl
  $ sudo /etc/init.d/apache2 restart


== Installation ==

Installing voipkookoo.module is very simple.  It requires a few configuration steps on your Drupal site to let it know how to reach your KooKoo account It also requires a few settings in your KooKoo account to make sure it knows which Drupal site to use.

Drupal configuration:

1. Install and enable voipkookoo.module

2. Set KooKoo as the default voip server
  - Go to admin/voip/servers

  - Click on KooKoo's "configure" link

  - Fill in the "KooKoo API Key" associated with your KooKoo account. API Key can be found in the your KooKoo account's "Dashboard"

  - Go back to admin/voip/servers

  - Select the 'KooKoo' option

  - Press the 'set default voip server' button


KooKoo configuration:

1. Go to the KooKoo website and login into your KooKoo account

2. Add a new phone number
   - Click on the "Dashboard" section

   - Click on the "Add New Number" button and follow the instructions

   - Associate your site URL with the new number. 
     Use http://mysite.com/voip/kookoo/callhandler/ (for clean URLs)
     or http://mysite.com/?q=voip/kookoo/callhandler/ (for non-clean URLs)

   - Press the "save" button

3. Enable outbound calls
   - By default outbound calling is disabled for all the KooKoo users. To 
     enable outbound for your account please send an email to support@kookoo.in
     with subject "Enable Outbound"

4. Configure KooKoo to send SMS messages

5. Configure KooKoo to receive SMS messages
  - Note that inbound SMS can only be received in Silver Egg and Golden Egg subscriptions.

  - Also, only one SMS code can be used per KooKoo account. You need to maintain multiple 
    KooKoo accounts to associate different codes with different URLs

  - Click on the "Settings" section.

  - Fill the "Application URL" field with
    http://mysite.com/voip/kookoo/callhandler/ (for clean URLs)
    or http://mysite.com/?q=voip/kookoo/callhandler/

  - Fill the "Keyword" field with the keyword that will be used to identify
    your application whenever an SMS is sent to KooKoo's central SMS number.
    More specifically, you should ask your users to send an SMS to 
    0-922-750-751-2 with "your_keyword <space> message" as a message.

  - Press the "Save" button


== Important notes ==

* KooKoo limits the amount of outbound calls to 50/day per number. It is
  possible to increase that limit by either making a special request to 
  KooKoo or using multiple phone lines.

* Due to TRAI regulations, calls are not allowed from 9pm to 9am IT.

* KooKoo does not support SMS sent to international numbers outside India

* The following commands are currently not supported:
  - VoipScript::addDial();
  - voip_hangup();
   

== About ==

This module was originally designed and implemented by Tamer Zoubi and Leo Burd under the sponsorship of Terravoz (http://terravoz.org).
