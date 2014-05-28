void RFduinoBLE_onAdvertisement(bool start)
{
}

void RFduinoBLE_onConnect()
{
  isConn = true;
}

void RFduinoBLE_onDisconnect()
{
  isConn = false;
}

// returns the dBm signal strength indicated by the receiver after connection (-0dBm to -127dBm)
void RFduinoBLE_onRSSI(int rssi)
{
}

void RFduinoBLE_onReceive(char *data, int len)
{
}
