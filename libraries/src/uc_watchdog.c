
#include<uc_watchdog.h>

void WDG_SetReload(WDG_TYPE* WDG, uint32_t value)
{
    CHECK_PARAM(PARAM_WDG(WDG));
    
    WDG->WIV = value;
}

void WDG_Cmd(WDG_TYPE *WDG, FunctionalState NewState)
{
    CHECK_PARAM(PARAM_WDG(WDG));
    CHECK_PARAM(PARAM_WDG_STATE(NewState));
    
    if(NewState)
    {
        WDG->CTR |= (WDG_ENABLE_MASK);
    }
    else
    {
        WDG->CTR &= ~(WDG_ENABLE_MASK);
    }
}

void WDG_FEED(WDG_TYPE *WDG)
{
    CHECK_PARAM(PARAM_WDG(WDG));
    
    WDG->WFD |= WDG_FEED_MASK;
}

