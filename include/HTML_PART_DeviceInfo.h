/* DeviceInfo HTML */ 

#ifndef HTML_PART_DeviceInfo_h
#define HTML_PART_DeviceInfo_h

const char PART_deviceinfo_html[] PROGMEM = R"rawliteral(
<div class = "border">
  <div class = "row">
    <div class = "innerl">
      <div class = "column1">
        <div class = "name-text">IP Address: </div>
        <div class = "name-text">MAC Address: </div>
        <div class = "name-text">Flash Usage: </div>
        <br>
        <div class = "name-text">Firmware Version: </div>
      </div>
    </div>
    
    <div class = "innerr">
      <div class = "column2">
        <div class = "host-text">%ipplaceholder%</div>
        <div class = "host-text">%macplaceholder%</div>
        <div class = "host-text">%memsketch%</div>
        <div class = "host-text">%FirmwareVer%</div>
      </div>
    </div>
  </div>
</div>
)rawliteral";

#endif  // HTML_PART_DeviceInfo_h