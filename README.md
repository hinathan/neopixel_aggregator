# neopixel_aggregator
External Component for ESPHome for building physical dashboards using addressable LEDs and the Neopixel library

Connect a WS2811 LED strand to your device with the LEDs arranged in a way which makes sense to you for the sensors and states you are trying to represent.

I used a laser cutter to etch an outline of my home's floorplan and assigned entity ids for each relevant location. That all gets stuffed into a suitably-sized shadow box frame purchased from Ikea but anything similar will do.

If you don't have access to a CNC or laser cutter, you can also drill holes with a hand drill and use hot glue to connect LEDs behind those holes. 


My yaml looks like this:


```
# standard declaration of a LED strand
light:
  - platform: neopixelbus
    type: RGB
    variant: WS2811
    pin: GPIO02
    num_leds: 40
    internal: true
    id: house_panel_leds
    name: "House Panel LEDs"

# makes a slider so you can adjust the brightness (in a dark bedroom, for example)
number:
  - platform: template
    name: "House Panel brightness"
    id: house_panel_brightness
    optimistic: true
    min_value: 0
    max_value: 255
    step: 1
    restore_value: true
    initial_value: 40
    set_action:
      then:
        # the force/delay/force part here is just to turn
        # on all the LEDs when you adjust the brightness,
        # which can help with some strands where some colors
        # only turn on above a certain brightness, ymmv.
        - lambda: |
            id(eventagg1)->set_force(true);
            id(eventagg1)->set_brightness(x/255.0);
        - delay: 500ms
        - lambda: |
            id(eventagg1)->set_force(false);
            id(eventagg1)->redraw();

# pull in this code
external_components:
  - source: github://hinathan/neopixel_aggregator@main

# associate the above LED strand with entities and colors
neopixel_aggregator:
  id: eventagg1
  neopixel_id: house_panel_leds
  brightness: 0.30
  heartbeat: 0
  leds:
    0:
      # heartbeat light
    1:
      - binary_sensor.water_meter_municipal_water_flowing:
          color: "#0000ff"
      - binary_sensor.hot_water_is_dispensing:
          color: "#ff0000"
    2:
      # main bedroom lights
      - light.master_bedroom_main_lights
      - light.master_bedroom_sconces
      - light.master_bedroom_accent_lights
    3:
      # main bedroom door
      - binary_sensor.ring_contact_sensor_64967:
          color: "#1121ff"
    4:
      - light.guest_bedroom_main_lights
    5:
      # guest bed door
      - binary_sensor.alarm_zone_5:
          color: "#d300bb"
    6:
      - light.upstairs_hallway_main_lights
      # upstairs motion sensor
      - binary_sensor.alarm_zone_24:
          color: "#00ff00"
    7:
      - light.alex_bedroom_main_lights

... etc
```