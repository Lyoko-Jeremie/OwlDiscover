// jeremie

#ifndef OWLDISCOVER_OWLCMD_H
#define OWLDISCOVER_OWLCMD_H

namespace OwlCmd {

    // follow table come from OwlAccessTerminal
    enum class OwlCmdEnum {
        ping = 0,
        emergencyStop = 120,
        unlock = 92,

        calibrate = 90,
        breakCmd = 10,
        takeoff = 11,
        land = 12,
        move = 13,
        rotate = 14,
        keep = 15,
        gotoCmd = 16,

        led = 17,
        high = 18,
        speed = 19,
        flyMode = 20,

        joyCon = 60,
        joyConSimple = 61,
        joyConGyro = 62,

    };
    enum class OwlCmdRotateEnum {
        cw = 1,
        ccw = 2,
    };
    enum class OwlCmdMoveEnum {
        up = 1,
        down = 2,
        left = 3,
        right = 4,
        forward = 5,
        back = 6,
    };

}

#endif //OWLDISCOVER_OWLCMD_H
