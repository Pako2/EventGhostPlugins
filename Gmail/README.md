**Introduction**  
This plugin has only one function and one action: it sends emails using the Google Gmail account.  
The plugin uses [OAuth](https://en.wikipedia.org/wiki/OAuth) authorization, so you do not need to save the credentials in open form anywhere !  


**ATTENTION!**  
This plugin requires the user to have an account on [Google Developers](https://developers.google.com/) !!!  

**Steps:**  
-----------------  
**A) Getting the client_secret.json file (it may be common to multiple Gmail plugins)**  
A.1) Use [this](https://console.developers.google.com/start/api?id=gmail) wizard to create or select a project in the Google Developers Console and automatically turn on the API. Click Continue, then Go to credentials  
A.2) On the Add credentials to your project page, click the Cancel button  
A.3) At the top of the page, select the OAuth consent screen tab. Select an Email address, enter a Product name if not already set, and click the Save button  
A.4) Select the Credentials tab, click the Create credentials button and select OAuth client ID  
A.5) Select the application type Other, enter the name "Gmail API Quickstart", and click the Create button  
A.6) Click OK to dismiss the resulting dialog  
A.7) Click the file_download (Download JSON) button to the right of the client ID  
A.8) Move this file to your "Credentials Folder" and rename it *client_secret.json*  
A.9) If you want to have multiple instances of the Gmail plugin for multiple Gmail accounts, copy the *client_secret.json* file to all "Credentials Folders"  
  
**B) Gmail plugin settings**  
B.1) Add the Gmail plugin to EventGhost  
B.2) In the main configuration dialog, set the path to the "Credentials Folder"  
B.3) Prefix setting is optional. This is especially important if you have multiple instances of the Gmail plugin for multiple Gmail accounts  
B.4) Press the OK button to close the configuration dialog  
B.5) If the *client_secret.json* file is in the "Credentials Folder", your default internet browser opens  
B.6) Select the appropriate Gmail account and allow email to be sent using the Gmail plugin  
![flow1](https://github.com/Pako2/EventGhostPlugins/raw/master/Gmail/Screenshots/flow1.png)  
![flow2](https://github.com/Pako2/EventGhostPlugins/raw/master/Gmail/Screenshots/flow2.png)  
![flow3](https://github.com/Pako2/EventGhostPlugins/raw/master/Gmail/Screenshots/flow3.png)  

This project still in its development phase.  
Latest development version is 0.0.0 .  
