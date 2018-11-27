Z-Stack-3.0.1_lchtime

zigbee for lchtime 
mesh 

// 硬件说明
P00 - K1 
P01 - K2
P02 - K3

P03 - LED1
P04 - LED2
P05 - LED5

P06 - COIL0
P12 - COIL1
P13 - COIL2

仅调试使用
P1.4 - TX
P1.5 - RX  

/* 一些说明 */ 

在on board.h 中有两个宏
SW_BYPASS_NV
SW_BYPASS_START
对应的两个按键
当按键开机时持续按下会触发一些功能,在配置按键时,这两个功能不用,一定要想办法避开,否则bug无限
SW_BYPASS_NV : 当开机时,持续按住,将不使用nv restore 将重启一个新网络
SW_BYPASS_START: 当开机时,持续按住, 设备再进入HOLD,意思是不会自动搜寻网络加入
当然,得看按键支持了么, 还得看这几个API有没被调用

在SAPI.c 第984行,有个HAL_KEY_SW_5  支持开机时,持续按住, 将不使用nv restore 并重启芯片, 需要使能SAPI_CB_FUNC 和 HAL_KEY