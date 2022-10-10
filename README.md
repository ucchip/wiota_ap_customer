# ucchip
ucchip is a fast-growing chip design company, which provides fast customization of low-cost, low power iot SoC chips and iot solutions for all industries.
We can provide variety kinds of chips such as GPRS/GPS, Wiota, LTE 
</br>
</br>

# About wiota_ap_customer
This project is the base station side in wiota protocol and runing in the 8088 chip.
</br>
</br>

# About UC8088

### Features
  ★ Frequency range: 65MHZ ~ 1GMHZ
  
  ★ Memory: 16MB NOR, 256KB SRAM, 128KB Data SRAM embedded in BSP 
  
  ★ Common peripherals supported</br>
      &emsp;2 x 16650 compatible UART.</br>
      &emsp;2 x 32 bits timer</br>
      &emsp;SPI Master, four peripheral signal selection</br>
      &emsp;4 PWM module, independent output. </br>
      &emsp;multiple gpio interface</br>
      &emsp;I2C interface.</br>
      &emsp;Three 12 bits ADC </br>
      &emsp;10 bits Voice DAC and auxiliary DAC</br>
  
  ★ Radio frequency features</br>
      &emsp; Sensitivity is lower than -145 dbm</br>
      &emsp; 17dbm PA built in</br>
      
  ★ RISCV 32bit CPU core with float operation unit FPU</br>
      &emsp; 160Mhz Maximum operation rate(except FPU and DSP communication)</br>
      &emsp; support mono periodic multiplication and hardware integer division</br>
      &emsp; support RISCV IMFC commands set and special expanded command</br>
      &emsp; FPU/DSP@131Mhz</br>
      
  ★ Clock</br>
      &emsp; Embedded with DCXO oscillator(required to connect external crystals)</br>
      &emsp; Embedded with 32Khz RC oscillator and 32Khz crystal oscillator(required to connect external crystals)</br>
      &emsp; Embedded with PLL to double frequency of DCXO clock</br>
      &emsp; support switch DCXO/PLL clock</br>
      
  ★ Power/chip management</br>
      &emsp; Embedded with DCDC converter</br>
      &emsp; Embedded with LDO, IO LDO</br>
      &emsp; Embedded with lithium battery</br>
      &emsp; support in-chip or out-chip temperature detection</br>
</br>      
      

# About WloTa Protocol
WloTa (Wide-range Intetnet Of Things communicAtion Protocol) protocol designed for the wide-area wireless loT communication is chinese independent intellectual property rights completely. 
The core characters include large coverage, low power consumption, a large number of connections, low cost, working at 230 MHZ to 470 MHZ in sub 1G spectrum.
So it is flexible and can meet the demand of Internet communication for many industries.</br>
WIoTa protocol design fully consider the diversified characteristics of IOT, adopting flexible architecture for adapting to various application requirements. 
According to the network size, multiple functional entity can be combined into a hardware entities.</br>

### Work mode
  ★ Synchronous mode</br>
      &emsp; AP network by star, point to multipoint. the system is high efficiency and has large capacity, can provide precise timing for IOT
      terminal, suitable for the special frequency band such as 230 MHZ. It support hierarchical network to enlarge the network coverage.</br>
      
  ★ Asynchronous mode</br>
      &emsp; Point to point, point to multipoint. The implementation in this mode is simple for these is no need to plan network.
      but the capacity is low than synchronous mode.
      WloTa communication protocol has also taken measures to optimize the design of the control signal and design conflict
      adjustable mechanism to maximize capacity in asynchronous mode .</br>
      
  ★ MESH mode</br>
      &emsp; This is no center mode. In asynchronous mode, the support of no center radio between IOT terminals is ebable.</br>
</br>


# Contact us
**Email: zheng.wang@ucchip.com**

