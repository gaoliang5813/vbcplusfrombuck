//
// Created by jackie on 12/15/20.
//

#include "XVideoEffect.h"

XVideoEffect::XVideoEffect()
    :mMode(0)
{
}

XVideoEffect::~XVideoEffect() {

}

void XVideoEffect::setMode(int mode) {
    mMode = mode;
}

int XVideoEffect::getMode() {
    return mMode;
}