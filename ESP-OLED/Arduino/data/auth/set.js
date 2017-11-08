var websock;
var utcSeconds;
var file = {};
var userdata = [];
var password = "FAKE";
var source;
var t;
var tt;
var count = 0;
var Modal;
var clss1=["btn-danger", "btn-success"];
var clss2=["glyphicon-remove", "glyphicon-ok"];


function onToggleCheckBox(id) {
    var el=document.querySelector('#'+id);
    var el2=el.getElementsByTagName("span")[0];
    var val = parseInt(el.value);
    el.classList.remove(clss1[val]);
    el2.classList.remove(clss2[val]);
    val = (val ? 0 : 1);
    el.classList.add(clss1[val]);
    el2.classList.add(clss2[val]);
    el.value = val;
  }

function setCheckBox(id, val_) {
    var el=document.querySelector('#'+id);
    var el2=el.getElementsByTagName("span")[0];
    var val = parseInt(val_);
    val = (val ? 0 : 1);
    el.classList.remove(clss1[val]);
    el2.classList.remove(clss2[val]);
    val = (val ? 0 : 1);
    el.classList.add(clss1[val]);
    el2.classList.add(clss2[val]);
    el.value = val;
  }


(function() {
    "use strict";
    
    /**
     * Bootstrap Modal Pure Js Class
     * 2015 (c) Daniel Vinciguerra 
     */
    Modal = function(el, options) {
        var self = this;
        
        this.el = document.querySelector(el);
        this.options = options;
        
        try {
            var list = document.querySelectorAll('#'+this.el.id+' [data-dismiss="modal"]');
            for (var x = 0; x < list.length; x++){
                list[x].addEventListener('click', function(e){ 
                    if(e) e.preventDefault();
                    self.hide();
                });
            }
        }
        catch(e){
            console.log(e);
        }
    };

    /**
     * Methods
     */
    Modal.prototype.show = function() {
        var self = this;
        // adding backdrop (transparent background) behind the modal
        if (self.options.backdrop) {
            var backdrop = document.getElementById('bs.backdrop');
            if (backdrop === null) {
                backdrop = document.createElement('div');
                backdrop.id = "bs.backdrop";
                backdrop.className = "modal-backdrop fade in";
                document.body.appendChild(backdrop);
            }
        }

        // show modal
        this.el.classList.add('in');
        this.el.style.display = 'block';
        document.body.style.paddingRight = '13px';
        document.body.classList.add('modal-open');
    };

    Modal.prototype.hide = function() {
        var self = this;
        // hide modal
        this.el.classList.remove('in');
        this.el.style.display = 'none';
        document.body.style = '';
        document.body.classList.remove('modal-open');

        // removing backdrop
        if (self.options.backdrop) {
            var backdrop = document.getElementById('bs.backdrop');
            if (backdrop !== null) document.body.removeChild(backdrop);
        }
    };
   
    /* Event */
    document.addEventListener('DOMContentLoaded', function(){
        var m = new Modal('#MyModal', {
            backdrop: true
        });
        document.getElementById('btn-open').addEventListener('click', function(e) {
          console.log("btn-open click !");
           if(e) e.preventDefault();
           m.show();
        });
        try {
            var lst = document.querySelectorAll('[data-fn="plus"]');
            for (var x = 0; x < lst.length; x++){
                lst[x].addEventListener('click', function(e){ 
                    if(e) e.preventDefault();
                    var el = this.parentElement.parentElement.getElementsByTagName("input")[0];
                    var val = el.value;
                    val++;
                    if (val > el.max){val = el.max;}
                    el.value = val;
                    el['custom-oldval'] = val;
                });
            }
        }      
        catch(e){
            console.log(e);
        }        
        try {
            var lst = document.querySelectorAll('[data-fn="minus"]');
            for (var x = 0; x < lst.length; x++){
                lst[x].addEventListener('click', function(e){ 
                    if(e) e.preventDefault();
                    var el = this.parentElement.parentElement.getElementsByTagName("input")[0];;
                    var val=el.value;
                    val--;
                    if (val<el.min){val=el.min;}
                    el.value = val;
                    el['custom-oldval']=val;
                });
            }
        }      
        catch(e){
            console.log(e);
        }        
        try {
            var lst = document.querySelectorAll('[data-inp="spinner"]');
            for (var x = 0; x < lst.length; x++){
                //lst[x]['custom-oldval'] = lst[x].value;//store old value
                lst[x].addEventListener('change', function(e){ 
                    if(e) e.preventDefault();
                    if (isNaN(parseInt(this.value, 10)))
                    {
                        this.value=this['custom-oldval'];
                    }
                    
                    else if (parseInt(this.value)>parseInt(this.max) || parseInt(this.value)>parseInt(this.max))
                    {
                        this.value=this['custom-oldval'];
                    }
                });
            }
        }      
        catch(e){
            console.log(e);
        }  
    });
})();


function start()
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
}


function listCONF(obj) {
  document.getElementById("inputtohide").value = obj.ssid;
  document.getElementById("wifipass").value = obj.pswd;
  document.getElementById("adminpwd").value = obj.apwd;
  document.getElementById("ntpserver").value = obj.ntpser;
  document.getElementById("intervals").value = obj.ntpint;
  document.getElementById("DropDownTimezone").value = obj.tz;
  document.getElementById("hostname").value = obj.hostnm;
  if (obj.addr) {
    document.getElementById("ip_static").value = obj.addr;
  }
  if (obj.gw) {
    document.getElementById("ip_gateway").value = obj.gw;
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
  dlAnchorElem.setAttribute("download", "esp-sensor-settings.json");
  document.getElementById("wifiLED").value = obj.wled;
  document.getElementById("fonts").value = obj.font;
  document.getElementById("contrast").value = obj.contrast;
  document.getElementById("contrast")['custom-oldval'] = obj.contrast;
  setCheckBox("sound_c", obj.sound_c);
  setCheckBox("sound_m", obj.sound_m);
  document.getElementById("bbt_channel").value = obj.chnnl;
  document.getElementById("bbt_cmdrsrc").value = obj.cmdrsrc;
  document.getElementById("bbt_msgrsrc").value = obj.msgrsrc;
  document.getElementById("bbt_token").value = obj.tkn;
}


function setFont()
{
  var sel=document.getElementById("fonts").value;
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
  document.getElementById("bbt_settings").style.display = "none";
  handleIP("none");
  document.getElementById("staticIP").style.display = "block";
}


function handleSTA() {
  document.getElementById("modeip").style.display = "block";
  document.getElementById("scanb").style.display = "inline";
  document.getElementById("hideBSSID").style.display = "block";
  document.getElementById("bbt_settings").style.display = "block";
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
  datatosend.addr = document.getElementById("ip_static").value;
  datatosend.gw = document.getElementById("ip_gateway").value;
  datatosend.mask = document.getElementById("ip_mask").value;
  datatosend.pswd = document.getElementById("wifipass").value;
  datatosend.ntpser = document.getElementById("ntpserver").value;
  datatosend.ntpint = document.getElementById("intervals").value;
  datatosend.tz = document.getElementById("DropDownTimezone").value;
  datatosend.hostnm = document.getElementById("hostname").value;
  datatosend.wled = document.getElementById("wifiLED").value;
  datatosend.font = document.getElementById("fonts").value;
  datatosend.contrast = document.getElementById("contrast").value;
  datatosend.sound_c = document.getElementById("sound_c").value;
  datatosend.sound_m = document.getElementById("sound_m").value;
  datatosend.chnnl = document.getElementById("bbt_channel").value;
  datatosend.cmdrsrc = document.getElementById("bbt_cmdrsrc").value;
  datatosend.msgrsrc = document.getElementById("bbt_msgrsrc").value;
  datatosend.tkn = document.getElementById("bbt_token").value;
  datatosend.apwd = a;

  websock.send(JSON.stringify(datatosend));
  var mm = new Modal('#MyModal2', { backdrop: true });
  mm.show();
  //$("#MyModal2").modal();
  location.reload();
}

function toInt(bl)
{
  return bl ? 1 : 0;
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
  document.getElementById("gate").innerHTML = obj.gw;
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
    else if (obj.command === "fontlist") 
    {
      fontlist = obj.fonts;
      var select = document.getElementById("fonts");
      var val=0;
      for (var i = 0; i < fontlist.length; i++)
      {
        select.options[select.options.length] = new Option(fontlist[i], val);
        val++;
      }
      //document.getElementById("loading-img").style.display = "none";
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
      websock.send("{\"command\":\"fontlist\"}");
      websock.send("{\"command\":\"getconf\"}");
      websock.send("{\"command\":\"gettime\"}");
      document.getElementById("loading-img").style.display = "none";  
    }
  };
}
