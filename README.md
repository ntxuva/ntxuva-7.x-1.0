ntxuva
====================
A Participatory Monitoring System for cities in Developing Countries based on Drupal, Mark-a-Spot and VoIP Drupal.

We've themed [Mark-a-Spot](https://www.markaspot.de/) and customized its Open311 API for integration with our tools. Thank you [Holger](https://github.com/markaspot) for all your help & support.

The full history of commits was lost, due to a wrong initial commit. But attribution to Holger's work in maintained in all files.

Ntxuva is the core system that supports [MOPA](http://www.mopa.co.mz/), a participatory monitoring system for waste management currently implemented in Maputo, Mozambique. 

# Current architecture of MOPA

# Justification

MOPA is a bootstrap project. It has been developed with the intention of delivering quick, yet customized, solutions in a project with low resources.

Technology development account for roughly 15% of all investments made by MOPA since 2013. 

Also, requirements have changed over time, and different developers have worked in the project. This lead to an over complexity of the architecture. If we knew everything we know today, most probably we would have done it differently. 

This document shares the current architecture of MOPA. Its purpose is to share the knowledge about what has been developed and how everything currently works. Finally, some thoughts are shared on how a future architecture could be deployed with a more homogeneous code base and technology stack.

----------
# High-level components
![Architecture](https://d2mxuefqeaa7sj.cloudfront.net/s_F0B74237D50C0EF50831F98E33FE11E8EA180F53070F9F7B8D0EFF63DFE6A789_1497725185882_MOPA+Architecture+-+Page+111.svg)

As depicted above, MOPA currently has the following high-level components:
**Administration systems**

- Back-end system (Database & Open311 API)

**User interfaces**

- Ticket management system
- Public Website
- USSD interface
- Mobile Application
- Facebook Application

**Auxiliary modules**

- SMS notification system
- PDF Report system
- SMS & USSD Gateway


## Administration systems 

**Back-end system (Database & Open311 API)**
**Description**
Includes admin interface for:

- service categories
- status taxonomy
- information about geographical location of containers
- registry of people to be notified via SMS

Saves all 311 requests and implements the Open311 GeoReport v2 API.

**Technology stack**
Drupal 7
Distribution: Mark-a-spot
LAMP (Linux, Apache, MySQL, PHP)

**Developed by**
Fork from open source project (http://www.mark-a-spot.de/en )
Improvements by WB team 

**Source code**
http://github.com/ntxuva/ntxuva-7.x-1.0

The Open311 API is implemented by the Open311 module, that you can find in the following directory: **profiles/markaspot/modules/mark_a_spot/modules/markaspot_open311**

The source code Open311 API Module is available here: https://github.com/ntxuva/ntxuva-7.x-1.0/tree/master/profiles/markaspot/modules/mark_a_spot/modules/markaspot_open311

**API Specifications**
The API follows the Open311 GeoReport v2 standard, as specified here: 
http://wiki.open311.org/GeoReport_v2/

Apart from Open311 GeoReport v2 specifications, we have developed some specific features to support the implementation in Maputo:

- Locations, which allows easy access to the locations were problems can occur (neighbourhood quarters, waste containers, etc.)
- People, which provides the list of people that need to be notified related to each service request
- Re-open issues, that allows users to re-open an issue when they do not agree that it was solved.
----------
## User interfaces

### Ticket management system
**Description**
Ticket management interface used by MOPA team @ CMM.

It allows registered users to:

- Update report status, providing additional information to citizen.
- View statistical data about platform usage.

This component uses the same Drupal instance as Back-end & Public website.

**Technology stack**
Drupal 7
Distribution: Mark-a-spot
LAMP (Linux, Apache, MySQL, PHP)

**Developed by**
Fork from open source project (http://www.mark-a-spot.de/en )
Improvements by WB team 

**Source code**
http://github.com/ntxuva/ntxuva-7.x-1.0

----------

### Public Website
**Description**


**Technology stack**
Drupal Mark-a-spot

**Developed by**
Fork from open source project (http://www.mark-a-spot.de/en )
Template by UX

**Source code**
http://github.com/ntxuva/ntxuva-7.x-1.0

The source code of the template is available here: https://github.com/ntxuva/ntxuva-7.x-1.0/tree/master/profiles/markaspot/themes/ntxuva

----------

### USSD interface
**Description**
The USSD interface allows users in the peri-urban area of Maputo to report problems to MOPA. It was designed to overcome the barrier of access to people that do not own smart phones. With four simple steps, users can submit their service request.

The locations where problems can exist were previously mapped by the MOPA team. You can submit problems related to waste containers or to a neighbourhood quarter. 

A service on the MOPA API provides the USSD interface with all available locations (http://www.mopa.co.mz/georeport/v2/locations.json). The USSD menu dynamically adapts to the locations provided by the API.

**Technology stack**
PHP

**Developed by**
UX

**Source code**
Not yet released.

----------

### Mobile Application
**Description**
Mobile application that allows people to submit reports and see reports already existing in the system. 

**Technology stack**

- Hybrid application
- Apache Cordova
- HTML5 + JavaScript

**Hosting**
Available @ [Google Play](https://play.google.com/store/apps/details?id=mz.co.mopa&utm_source=global_co&utm_medium=prtnr&utm_content=Mar2515&utm_campaign=PartBadge&pcampaignid=MKT-Other-global-all-co-prtnr-py-PartBadge-Mar2515-1)

**Developed by**
UX

**Source code**
Not released.

----------

### Messenger Bot
**Description**
The Messenger Bot allows people to use Facebook and Telegram interfaces to report problems to MOPA. 

It was created in June 2017 in a prototype form and it replicates the USSD work flow, leveraging on the advantages enabled by these new services. The main advantages are:

- People can submit their exact location, responding to a request to send their location that activates Facebook Messenger and Telegram location interfaces.
- People can send a picture of the problem.

In the specific case of Facebook Messenger, and in the case of Mozambique, there is the additional advantage of it being free. Users pay no traffic when using Facebook.

**Technology stack**
On its initial version, the Bots were developed on [Flowxo](https://flowxo.com/). 

In the future, if proven successful, an open source development can be easily deployed.

**Hosting**
[Flowxo](https://flowxo.com/)

**Developed by**
World Bank

**Source code**
- Catch all trigger - https://flowxo.com/share/v9q2j37z
- Request problem category & location - https://flowxo.com/share/pdkxj9br
- Receive location - https://flowxo.com/share/m6xgpxke
- Receive picture - https://flowxo.com/share/gpp3qj7n
- Finalize submission - https://flowxo.com/share/j2ka2e9d
----------
## Auxiliary modules

### SMS notification system
**Description**
This module uses the issue list from the Open311 GeoReport v2 API to track changes in requests and notify users when there’s something new. 

Use cases for notifications:

- New report - Notify all people involved in solving the problem (Municipality, Service Providers, Neighbourhood secretary)
- Updates in report status - Notify Citizen + Solvers.

This allows solvers to know what’s happening in their areas in real time. It also allows the citizen who reported the problem to have feedback on new status. 

To minimize changes at the API level, the flow works as follows:

- Compare updated_datetime to requested_datetime. 
  - If different, check if person was already notified before (DB table)
    - If yes, ignore.
    - If not, according to status, send message to Solvers only (if problem status is New) or Solvers + Citizen (for all other problem statuses).
  - If same, ignore. 

**Technology stack**
Python + Apache + MySQL
Flask

**Developed by**
UX

**Source code**
https://github.com/ntxuva/mopa-utils/

----------

### PDF Report System
**Description**
This module automatically analyses data from Open311 API and generates PDF reports send to the municipality. 

This allows automatic distribution of PDF files by email. It provides relevant information to specific user groups. 

Available reports:

- Daily reports, with lists of all problems, split by Municipal District.
- Weekly reports, with statistical information, lists and summary of the week. 
- Monthly reports, with number of reports, monthly evolution, quality of response indicators. 

All reports are created in HTML, and then exported to PDF by using [xhtml2pdf](http://www.xhtml2pdf.com/).

Monthly reports are created  by using [Pandas](http://pandas.pydata.org/), [Seaborn](https://seaborn.pydata.org/), and [Matplotlib](https://matplotlib.org/) libraries.

**Technology stack**
Python + Apache + MySQL
Flask

**Developed by**
UX

**Source code**
https://github.com/ntxuva/mopa-utils/

----------

