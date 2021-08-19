

- _sendEvent:(NWEvent *)anEvent
 /*
  * Panel overrides _sendEvent in order to set a trackingRect for fading. 
  */
{
    register int    type;

    type = anEvent->type;
 /* Must cache event type since _sendEvent may change it */
    [super _sendEvent:anEvent];
    if ((type == NW_LMOUSEDOWN) && fadeEnabled)
	[self _setFadeRect];
    return self;
}
