# BluePillDemo

The Blue Pill is a very cheap bare bones development board containing a STM32F103C8 ARM Coretex M3 processor with 64 kBytes of flash and 20 kBytes of RAM. This makes it an excellent piece of hardware to get into professional standard embedded programming on an ARM microcontroller on the cheap. There are many examples of how to get started with this board available but almost all of them use the Arduino environment. While this is fine, and an easy way to start embedded programming, it has its limitations. The Arduino way shelters you from getting close to the processor with an easy to use interface. This restricts what you can do in the code and makes it inefficient. Because of this the Arduino environment is almost never used professionally in industry. For anyone looking to make a career of embedded development it is necessary to move on.

The manufacturer of the processor on the Blue Pill board, ST, provide a compiler/IDE environment that is widely used in industry. The latest incarnation is called STM32CubeIDE and is a free unrestricted download from ST. However, it is a daunting bit of software supporting a huge range of processors and all their capabilities and is not easy at all to get going with. To help this process this repo contains a collection of small projects that can be directly imported into STM32CubeIDE. 

Are you fed up with searching the internet for example code that is poorly written, full of messy commented out code, doesn't build, doesn't work or is cluttered with all sort of guff that hides what you are trying to find out to do? Me too. Therefore these projects start off at the lowest level being SIMPLE. The lowest level projects shows how to configure and then exercise one (usually!) peripheral on the processor. Then moving on there are some common simple driver examples that exercise the peripherals, like I2C, UART or SPI, by communicating with external devices. There are projects that integrate FreeRTOS, at the lowest level doing only that and nothing else, to keep that as simple as possible. Then there are projects that integrate FreeRTOS with peripheral drivers like the 16x2 HD44780 text display or third party middleware like the FatFS library. At the highest level are the SIM800L modem examples (these incorporate drivers (GPIO and UART), FreeRTOS, middleware (the modem driver), application code (MQTT library) and html and javascript running on a webserver) and the MAX30102 pulse oximeter with graphical display project. 

Another bugbear of internet sourced example embedded code is that it's often just the code and you are left clueless on how to wire up the hardware to run it. These projects all have a schematic where components external to the Blue Pill board are used.

In the project there is a readme file describing the project, a circuit diagram if required and the STM32CubeIDE hardware configuration file that is used to set up the processor's hardware and any peripheral used (.ioc file). The example code then makes calls into ST's HAL library to operate the peripheral.

Once the code is built, flashing and debugging needs a ST-LINK device connected to the Serial Wire 4 pin header at the end of the Blue Pill board. These are cheaply available on ebay for a few £€$, sometimes supplied together with a Blue Pill board. You need 4 female to female jumper cables to connect the two. Note: The ST-LINK v2 debugger will not connect to Blue Pill boards with fake processors on them. Some Blue Pill boards are supplied with non-genuine or cloned STM32F103C8T6 processors.

Any requests can be added as an issue to this repo. I'll create a project if I have time and the extra hardware is not too expensive for me to buy.

The Seatalk projects are only of interest for interfacing with the Seatalk marine data network.

# Creating a new Blue Pill project

A brief explanation of how to create a new project for the Blue Pill board in STM32CubeIDE using a ST-LINK device for flashing/debugging is shown below:

File|New|STM32 Project

In Target Selection type STM32F103C8 in Part Number Search

Select this processor when it is listed on the right, Next

Enter Project Name, select C++ if you need it, leave other selections default, Finish

In 'Project Name'.ioc editor Select Pinout & Configuration tab

In Pinout & Configuration choose Categories then expand System Core then select SYS

Under Debug select Serial Wire

In System Core select RCC

In both High Speed Clock (HSE) and Low Speed Clock (LSE) select Crystal/Ceramic Resonator

Select Clock Configuration Tab

In System Clock Mux select PLLCLK

In PLL Source Mux select HSE

In PLLMul select X9

In APB1 Prescalar select /2


For projects using USB, ADC or RTC further clock configuration is required. See example projects.
