var websock;
var utcSeconds;
var file = {};
var userdata = [];
var pins = ["A0","D0","D1","D2","D3","D4","D5","D6","D7","D8"];
var password = "FAKE";
var source;
//var millisecondsToWait = 500;
var t;
var tt;
var count = 0;


//(function(){
//    var tId = setInterval(function(){if(document.readyState == "complete") onComplete()},50);
//    function onComplete(){
//        clearInterval(tId);    
//        console.log("loaded: ",new Date().getTime().toString());
//        
//
//        //setTimeout(function(){wait();}, millisecondsToWait);
//    };
//})()


$(window).load(function()
{
    console.log("Window loaded   ",new Date().getTime().toString());

        if (!!window.EventSource) {
           source = new EventSource('/events');
           source.addEventListener('open', function(e) {
              console.log("Events connected "+new Date().getTime().toString());
           }, false);
           source.addEventListener('error', function(e) {
              if (e.target.readyState != EventSource.OPEN) {
                 console.log("Events disconnected");
              }
            }, false);
            source.addEventListener('message', function(e)
            {
               password = e.data;
               console.log("Event message   ",new Date().getTime().toString());
               setWebsock();
               source.close();
            }, false);
        }     
    
  
});


function listCONF(obj) {
  document.getElementById("inputtohide").value = obj.ssid;
  document.getElementById("wifipass").value = obj.pswd;
  document.getElementById("adminpwd").value = obj.adminpwd;
  document.getElementById("ntpserver").value = obj.ntpserver;
  document.getElementById("intervals").value = obj.ntpinterval;
  document.getElementById("DropDownTimezone").value = obj.timezone;
  document.getElementById("hostname").value = obj.hostnm;
  if (obj.address) {
    document.getElementById("ip_static").value = obj.address;
  }
  if (obj.gateway) {
    document.getElementById("ip_gateway").value = obj.gateway;
  }
  if (obj.mask) {
    document.getElementById("ip_mask").value = obj.mask;
  }
  if (obj.wmode === "1") {
    document.getElementById("wmodeap").checked = true;
    handleAP();
  } else {
    document.getElementById("wifibssid").value = obj.bssid;
    handleSTA();
  }
  if (obj.ipmode === "0") {
    document.getElementById("ipmodesta").checked = true;
    handleIP("block");
  }
  var dataStr = "data:text/json;charset=utf-8," + encodeURIComponent(JSON.stringify(obj, null, 2));
  var dlAnchorElem = document.getElementById("downloadSet");
  dlAnchorElem.setAttribute("href", dataStr);
  dlAnchorElem.setAttribute("download", "esp-io-settings.json");
  var gpios = obj.gpios;
  for (var i = 0; i < 10; i++)
  {
    var pin=gpios[i][0];
    var enab=gpios[i][1];
    document.getElementById("enabled"+pin).checked=enab;
    document.getElementById("title"+pin).value=gpios[i][2];
    document.getElementById("out"+pin).checked=gpios[i][3];
    document.getElementById("pullup"+pin).checked=gpios[i][4];
    document.getElementById("def1_"+pin).checked=gpios[i][5];
    if (gpios[i][3]) {showBox2("xxxx"+pin);}
    else             {showBox("xxxx"+pin);}
    var row = document.getElementById("pin"+pin);
    if (enab) {row.className="success";}
    else      {row.className="danger";}
  }
  document.getElementById("wifiLED").value = obj.wifiled;
  ledPin();
}


function ledPin(){
  var sel=document.getElementById("wifiLED").value;
  if (sel=="D0")
  {
    document.getElementById("pinD0").style.display = "none";
    document.getElementById("pinD4").style.display = "table-row";
  }
  else if (sel=="D4")
  {
    document.getElementById("pinD0").style.display = "table-row";
    document.getElementById("pinD4").style.display = "none";
  }
  else
  {
    document.getElementById("pinD0").style.display = "table-row";
    document.getElementById("pinD4").style.display = "table-row";
  }
}


function browserTime() {
  var today = new Date();
  document.getElementById("rtc").innerHTML = today;
}


function deviceTime() {
  var t = new Date(0); // The 0 there is the key, which sets the date to the epoch
  t.setUTCSeconds(utcSeconds);
  var d = t.toUTCString();
  document.getElementById("utc").innerHTML = d;
  utcSeconds = utcSeconds + 1;
}


function syncBrowserTime() {
  var d = new Date();
  var timestamp = Math.floor((d.getTime() / 1000) + ((d.getTimezoneOffset() * 60) * -1));
  var datatosend = {};
  datatosend.command = "settime";
  datatosend.epoch = timestamp;
  websock.send(JSON.stringify(datatosend));
  location.reload();
}


function handleIP(mode) {
  //alert("IP mode = "+mode.toString());
  document.getElementById("staticIP").style.display = mode;
  document.getElementById("staticGW").style.display = mode;
  document.getElementById("staticMask").style.display = mode;
}


function handleAP() {
  document.getElementById("modeip").style.display = "none";
  document.getElementById("scanb").style.display = "none";
  document.getElementById("ipmodedyn").checked = true;
  document.getElementById("hideBSSID").style.display = "none";
  handleIP("none");
  document.getElementById("staticIP").style.display = "block";
}


function handleSTA() {
  document.getElementById("modeip").style.display = "block";
  document.getElementById("scanb").style.display = "inline";
  document.getElementById("hideBSSID").style.display = "block";
  document.getElementById("staticIP").style.display = "none";
}


function changeColor(id)
{
  var row = document.getElementById("pin"+id);
  if (document.getElementById("enabled"+id).checked) 
  {
      row.className="success";
  }
  else 
  {
      row.className="danger";
  }
}    

function showBox(name) {
  var id = "pullup"+name.substring(4);
  if (id != "pullupA0" && id != "pullupD0") {document.getElementById(id).style.display = "inline";}
  id = "def1_"+name.substring(4);
  document.getElementById(id).style.display = "none";
}


function showBox2(name) {
  var id = "def1_"+name.substring(4);
  if (id != "pullupA0") {document.getElementById(id).style.display = "inline";}
}


function hideBox(name) {
  var id = "pullup"+name.substring(4);
  document.getElementById(id).style.display = "none";
  id = "def1_"+name.substring(4);
  if (id != "pullupA0") {document.getElementById(id).style.display = "inline";}
}


function listSSID(obj) {
  var select = document.getElementById("ssid");
  for (var i = 0; i < obj.ssid.length; i++) {
    var opt = document.createElement("option");
    opt.value = obj.ssid[i];
    opt.bssidvalue = obj.bssid[i];
    opt.innerHTML = obj.ssid[i];
    select.appendChild(opt);
  }
  document.getElementById("scanb").innerHTML = "Re-Scan";
  listBSSID();
}


function listBSSID() {
  var select = document.getElementById("ssid");
  document.getElementById("wifibssid").value = select.options[select.selectedIndex].bssidvalue;
}


function scanWifi() {
  websock.send("{\"command\":\"scan\"}");
  document.getElementById("scanb").innerHTML = "...";
  document.getElementById("inputtohide").style.display = "none";
  var node = document.getElementById("ssid");
  node.style.display = "inline";
  while (node.hasChildNodes()) {
    node.removeChild(node.lastChild);
  }
}


function saveConf() 
{
  var ssid;
  var a = document.getElementById("adminpwd").value;
  if (a === null || a === "") {
    alert("Administrator Password cannot be empty");
    return;
  }
  if (document.getElementById("inputtohide").style.display === "none") {
    var b = document.getElementById("ssid");
    ssid = b.options[b.selectedIndex].value;
  } else {
    ssid = document.getElementById("inputtohide").value;
  }
  var datatosend = {};
  datatosend.command = "configfile";
  var wmode = "0";
  if (document.getElementById("wmodeap").checked) {
    wmode = "1";
    datatosend.bssid = document.getElementById("wifibssid").value = 0;
  } else {
    datatosend.bssid = document.getElementById("wifibssid").value;
  }

  var ipmode = "0";
  if (document.getElementById("ipmodedyn").checked) {
    ipmode = "1";
  } 
  datatosend.ssid = ssid;
  datatosend.wmode = wmode;
  datatosend.ipmode = ipmode;
  datatosend.address = document.getElementById("ip_static").value;
  datatosend.gateway = document.getElementById("ip_gateway").value;
  datatosend.mask = document.getElementById("ip_mask").value;
  datatosend.pswd = document.getElementById("wifipass").value;
  datatosend.ntpserver = document.getElementById("ntpserver").value;
  datatosend.ntpinterval = document.getElementById("intervals").value;
  datatosend.timezone = document.getElementById("DropDownTimezone").value;
  datatosend.hostnm = document.getElementById("hostname").value;
  datatosend.wifiled = document.getElementById("wifiLED").value;
  datatosend.adminpwd = a;

  var gpios = new Array(10);
  for (var i = 0; i < 10; i++)
  {
    gpios[i] = new Array(6);
    var pin = pins[i];
    gpios[i][0] = pin;
    gpios[i][1] = document.getElementById("enabled"+pin).checked;
    gpios[i][2] = document.getElementById("title"+pin).value;
    gpios[i][3] = document.getElementById("out"+pin).checked;
    gpios[i][4] = document.getElementById("pullup"+pin).checked;
    gpios[i][5] = document.getElementById("def1_"+pin).checked;
  }
  datatosend.gpios = gpios;
  websock.send(JSON.stringify(datatosend));
  $("#myModal2").modal();
  location.reload();
}


function backupset() {
  var dlAnchorElem = document.getElementById("downloadSet");
  dlAnchorElem.click();
}


function restoreSet() {
  var input = document.getElementById("restoreSet");
  var reader = new FileReader();
  if ("files" in input) {
    if (input.files.length === 0) {
      alert("You did not select file to restore");
    } else {
      reader.onload = function() {
        var json;
        try {
          json = JSON.parse(reader.result);
        } catch (e) {
          alert("Not a valid backup file");
          return;
        }
        if (json.command === "configfile") {
          var x = confirm("File seems to be valid, do you wish to continue?");
          if (x) {
            websock.send(JSON.stringify(json));
            alert("Device now should reboot with new settings");
            location.reload();
          }
        }
      };
      reader.readAsText(input.files[0]);
    }
  }
}


function colorStatusbar(ref) {
  var percentage = ref.style.width.slice(0, -1);
  if (percentage > 50) ref.className = "progress-bar progress-bar-success";
  else if (percentage > 25) ref.className = "progress-bar progress-bar-warning";
  else ref.class = "progress-bar progress-bar-danger";
}


function refreshStats() {
  websock.send("{\"command\":\"status\"}");
  document.getElementById("status").style.display = "block";
  document.getElementById("refstat").style.display = "none";
  document.getElementById("hidstat").style.display = "inline";
}


function hideStats() {
  document.getElementById("status").style.display = "none";
  document.getElementById("hidstat").style.display = "none";
  document.getElementById("refstat").style.display = "inline";
}


function listStats(obj) {
  document.getElementById("chip").innerHTML = obj.chipid;
  document.getElementById("cpu").innerHTML = obj.cpu + " Mhz";
  document.getElementById("heap").innerHTML = obj.heap + " Bytes";
  document.getElementById("heap").style.width = (obj.heap * 100) / 81920 + "%";
  colorStatusbar(document.getElementById("heap"));
  document.getElementById("flash").innerHTML = obj.availsize + " Bytes";
  document.getElementById("flash").style.width = (obj.availsize * 100) / 1044464 + "%";
  colorStatusbar(document.getElementById("flash"));
  document.getElementById("spiffs").innerHTML = obj.availspiffs + " Bytes";
  document.getElementById("spiffs").style.width = (obj.availspiffs * 100) / obj.spiffssize + "%";
  colorStatusbar(document.getElementById("spiffs"));
  document.getElementById("ssidstat").innerHTML = obj.ssid;
  document.getElementById("ip").innerHTML = obj.ip;
  document.getElementById("gate").innerHTML = obj.gateway;
  document.getElementById("mask").innerHTML = obj.netmask;
  document.getElementById("dns").innerHTML = obj.dns;
  document.getElementById("mac").innerHTML = obj.mac;
}


function setWebsock()
{
  websock = new WebSocket("ws://" + window.location.hostname + "/ws");
  websock.onopen = function(evt) {};
  websock.onclose = function(evt) {};
  websock.onerror = function(evt) {
    console.log(evt);
  };
  websock.onmessage = function(evt) {
    var obj = JSON.parse(evt.data);
    if (obj.command === "ssidlist") 
    {
      listSSID(obj);
    }
    else if (obj.command === "configfile")
    {
      listCONF(obj);
    }
    else if (obj.command === "gettime")
    {
      utcSeconds = obj.epoch;
    }
    else if (obj.command === "status")
    {
      listStats(obj);
    }
    else if (obj.command === "password")
    {
      websock.send("{\"command\":\"password\",\"password\":\""+password+"\"}");
    }
    else if (obj.command === "authorized")
    {
      t = setInterval(browserTime, 1000);
      tt = setInterval(deviceTime, 1000);
      websock.send("{\"command\":\"getconf\"}");
      websock.send("{\"command\":\"gettime\"}");
      document.getElementById("loading-img").style.display = "none";  
    }
  };
}


//function wait()
//{
//  console.log("Loop count: "+count.toString());
//  if (password == "FAKE")
//  {
//    count+=1;
//    if (count<10){setTimeout(function(){wait();}, millisecondsToWait);}
//    else {setWebsock();}
//  }
//  else
//  {
//    console.log("Password retrieved: "+new Date().getTime().toString());
//    source.close();
//    setWebsock();
//  }
//}
