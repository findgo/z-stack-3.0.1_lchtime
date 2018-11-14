Z-Stack-3.0.1_lchtime

zigbee for lchtime 
mesh 


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