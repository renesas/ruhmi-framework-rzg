# RZ/G3E -EVKIT Quick Setup Overview  

## Preparing RZ/G3E-EVKIT and the system configuration  

## Target Environment
- **Board**
•	[RZ/G3E-EVKIT](https://www.renesas.com/en/design-resources/boards-kits/rz-g3e-evkit)  
- **Software**
  - RZ/G3E Ethos-U Package (including the RZ/G3E RUHMI AI Framework)
  - RUHMI AO compiler for host
  
- **Peripheral Devices**
  - USB Camera
  - HDMI Display
  - microSD card (optional, used for boot media)

The following figure shows the hardware and system configuration of the Image Classification application on RZ/G3E.
![G3E operation env](../docs/assets/app-system-config_img.png)


### Connecting and Powering Up the EK-RA8P1 Board  
1.	Attach the Camera module   
2.	Attach the display  
3.	Connect the USB-C  cable to USB-C  (J10) of the EK-RA8P1 board.  
4.	Connect the other end of this cable to the USB port of the host PC. When powered, the white LED near the center of the board (the “dash” in the EK-RA8P1 name) will light up.   

![](../docs/assets/EK_RA8P1_buringup.GIF)

## Downloading sample AI Application and run  


