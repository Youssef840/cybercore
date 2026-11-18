#include "screens/Modules.h"

// ---- leaf screen instances --------------------------------------------------
WifiScanScreen wifiScanScreen;
WifiAuditScreen wifiAuditScreen;
ChannelScreen channelScreen;
DeauthScreen deauthScreen;
EvilTwinScreen evilTwinScreen;
PacketMonitorScreen packetMonitorScreen;
BleScanScreen bleScanScreen;
BleRadarScreen bleRadarScreen;
I2cScreen i2cScreen;
AdcScreen adcScreen;
FlashlightScreen flashlightScreen;
StopwatchScreen stopwatchScreen;
SnakeScreen snakeScreen;
QrScreen qrScreen;
NetToolsScreen netToolsScreen;
SysInfoScreen sysInfoScreen;
SettingsScreen settingsScreen;

#include "screens/Extras.h"

#define N(a) (sizeof(a) / sizeof(a[0]))

// ---- category: Wi-Fi --------------------------------------------------------
static const GridItem kWifi[] = {
    {"ماسح الشبكات", icon::RADAR, &wifiScanScreen},
    {"تدقيق الحماية", icon::SHIELD, &wifiAuditScreen},
    {"محلل القنوات", icon::GRAPH, &channelScreen},
    {"كاشف الفصل", icon::ALERT, &deauthScreen},
    {"كاشف التوأم", icon::WIFI, &evilTwinScreen},
    {"محلل الحركة", icon::EYE, &packetMonitorScreen},
    {"اتصال وادوات", icon::BOLT, &netToolsScreen},
    {"شبكات مخفية", icon::EYE, hiddenNetworks()},
    {"PMF / WPS", icon::SHIELD, beaconAudit()},
    {"فحص خدمات", icon::BOLT, serviceCheck()},
    {"تشخيص DNS", icon::GRAPH, dnsCheck()},
    {"مراقب التغير", icon::EYE, wifiWatch()},
    {"رؤوس HTTP", icon::SHIELD, httpCheck()},
};
Grid wifiGrid("ادوات الواي فاي", kWifi, N(kWifi));

// ---- category: Bluetooth ----------------------------------------------------
static const GridItem kBle[] = {
    {"ماسح بلوتوث", icon::BLUETOOTH, &bleScanScreen},
    {"رادار بلوتوث", icon::RADAR, &bleRadarScreen},
    {"مراقب جهاز", icon::EYE, bleTracker()},
    {"تحليل اعلان", icon::INFO, bleInspector()},
};
Grid bleGrid("بلوتوث", kBle, N(kBle));

// ---- category: Hardware lab -------------------------------------------------
static const GridItem kHw[] = {
    {"ماسح I2C", icon::CHIP, &i2cScreen},
    {"مقياس جهد", icon::BOLT, &adcScreen},
    {"كشاف", icon::LIGHT, &flashlightScreen},
};
Grid hwGrid("المعمل", kHw, N(kHw));

// ---- category: Tools --------------------------------------------------------
static const GridItem kTools[] = {
    {"مختبر متقدم", icon::SHIELD, advancedLab()},  {"رمز QR", icon::QR, &qrScreen},
    {"مؤقت", icon::TIMER, &stopwatchScreen},       {"افعى", icon::GAME, &snakeScreen},
    {"ملفات SD", icon::APPS, sdFiles()},           {"ساعة ومنبه", icon::TIMER, clockScreen()},
    {"خيارات الادوات", icon::GEAR, toolOptions()},
};
Grid toolsGrid("ادوات", kTools, N(kTools));

// ---- main menu --------------------------------------------------------------
static const GridItem kMain[] = {
    {"الواي فاي", icon::WIFI, &wifiGrid},
    {"بلوتوث", icon::BLUETOOTH, &bleGrid},
    {"المعمل", icon::CHIP, &hwGrid},
    {"ادوات", icon::APPS, &toolsGrid},
    {"معلومات النظام", icon::INFO, &sysInfoScreen},
    {"الاعدادات", icon::GEAR, &settingsScreen},
};
Grid mainMenu("القائمة الرئيسية", kMain, N(kMain));

Screen* rootScreen() { return &mainMenu; }
