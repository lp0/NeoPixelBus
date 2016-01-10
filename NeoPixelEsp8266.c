/*
NeoPixelEsp8266.c - NeoPixel library helper functions for Esp8266 using cycle count
Copyright (c) 2015 Michael C. Miller. All right reserved.

This library is free software; you can redistribute it and/or
modify it under the terms of the GNU Lesser General Public
License as published by the Free Software Foundation; either
version 2.1 of the License, or (at your option) any later version.

This library is distributed in the hope that it will be useful,
but WITHOUT ANY WARRANTY; without even the implied warranty of
MERCHANTABILITY or FITNESS FOR A PARTICULAR PURPOSE.  See the GNU
Lesser General Public License for more details.

You should have received a copy of the GNU Lesser General Public
License along with this library; if not, write to the Free Software
Foundation, Inc., 51 Franklin St, Fifth Floor, Boston, MA  02110-1301  USA
*/

#include <Arduino.h>
#include <eagle_soc.h>

#define NEO_KHZ800  0x02 // 800 KHz datastream
#define NEO_IRQLOCK 0x40 // IRQs will be locked on uart fifo writing

void ICACHE_RAM_ATTR esp8266_uart1_send_pixels(uint8_t* pixels, uint8_t* end, uint8_t flags)
{
    const uint8_t _uartData[4] = { 0b00110111, 0b00000111, 0b00110100, 0b00000100 };
    const uint8_t _uartFifoTrigger = 124; // tx fifo should be 128 bytes. minus the four we need to send
    uint32_t savedPS;
    int i;

    do
    {
        uint8_t subpix = *pixels++;
        uint8_t buf[4] = { _uartData[(subpix >> 6) & 3], 
            _uartData[(subpix >> 4) & 3], 
            _uartData[(subpix >> 2) & 3], 
            _uartData[subpix & 3] };

        // now wait till this the FIFO buffer has room to send more
        while (((U1S >> USTXC) & 0xff) > _uartFifoTrigger);

        if (flags & NEO_IRQLOCK)
        {
            savedPS = xt_rsil(15); // stop other interrupts
        }

        for (uint8_t i = 0; i < 4; i++)
        {
            // directly write the byte to transfer into the UART1 FIFO register
            U1F = buf[i];
        }

        if (flags & NEO_IRQLOCK)
        {
            xt_wsr_ps(savedPS); // reenable other interrupts
        }

    } while (pixels < end);

    // append 50us data latch to the buffer to ensure correct timing
    for (i = 0; i < ((flags & NEO_KHZ800) ? 20 : 10); i++) {
        while (((U1S >> USTXC) & 0xff) > _uartFifoTrigger);

        if (flags & NEO_IRQLOCK)
        {
            savedPS = xt_rsil(15); // stop other interrupts
        }

        U1F = 0;
        U1F = 0;
        U1F = 0;
        U1F = 0;

        if (flags & NEO_IRQLOCK)
        {
            xt_wsr_ps(savedPS); // reenable other interrupts
        }
    }
}

