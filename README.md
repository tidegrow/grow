# Tidegrow Grow

Tidegrow Grow is an experimental low-power grow light system. Boards can be chained to create custom panels.

![Grow](https://github.com/tidegrow/grow/blob/d1d28b5c89ea32b7ace8af1ea5e130b1900a3061/grow.png)

This repo contains schematics, PCB layouts, firmware and 3D-printable components.

Find more information on the [Grow product page](https://machdyne.com/product/tidegrow-grow/).

## Pinouts

### Zwölf

Grow has a [Zwölf](https://github.com/machdyne/zwolf) MCU socket that can be used to implement a timer, PWM dimming, I2C remote control, etc.

| Pin | Signal | Description |
| --- | ------ | ----------- |
| 1 | SCL | I2C clock |
| 2 | SDA | I2C data |
| 3 | EN | Float = ON, Low = OFF |
| 4 | TEMP | Board temperature (0V - 1.1V) |
| 5 | GND | Ground |
| 6 | 3V3 | Power (3.3V) |

### I2C Headers

Grow has two I2C headers for optional remote control and I2C chaining.

```
SCL SDA GND GND
```

## Notes

These are being tested in 0.6W and 1.2W configurations. The higher power can be acheived by changing R6 from a 2ohm to 1ohm resistor.

The footprints support either four LM301B or four 2835 LEDs.

## LLM-generated code

To the extent that there is LLM-generated code in this repo, it should be space indented. Any space indented code should be carefully audited and then converted to tabs (eventually). 

## License

The contents of this repo are released under the [Lone Dynamics Open License](LICENSE.md).

Note: You can use these designs for commercial purposes but we ask that instead of producing exact clones, that you either replace our trademarks and logos with your own or add your own next to ours.
