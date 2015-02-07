#include <framework/Framework.h>
//#ifndef TARGET_IS_BLIZZARD_RB1
//#include <Ethernet.h>
//#endif

void setup()
{
  // [Default    ]
  //Controller::getInstance().setup(115200);
  // [UART0_baud_rate, LPRF, [true: target, false: controller], a unique identification number]
  // [Target     ]
  Controller::getInstance().setup(115200, true, true,  0x7CD);
  // [Controllers]
  //Controller::getInstance().setup(115200, true, false, 0x8D4);
  //Controller::getInstance().setup(115200, true, false, 0x7EF);
  //Controller::getInstance().setup(115200, true, false, 0x8B3);
}

void loop()
{
  Controller::getInstance().loop();
}
