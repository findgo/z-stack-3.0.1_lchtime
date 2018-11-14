Z-Stack-3.0.1_lchtime

zigbee for lchtime 
mesh 


ZNP
0. UART  
目前只支持
TX -- P1.5
RX -- P1.4
需要改程序
TX -- P0.3
RX -- P0.2

引脚说明

1. PA CC2592 --> 开启方式 HAL_PA_LNA_CC2592
P0.7 -- HGM
P1.0 -- LNA_EN
P1.1 -- PA-EN

// HAL_KEY  HAL_LED未开启,引脚规划
LED3 -- P1.4
KEY  --  P2.0??

2. PA FRX2401C -->开启方式 HAL_PA_LNA USER_HAL_PA_LNA_RFX2401C
TXEN -- P1.5
RXEN -- P1.4

NOTE: FRX24