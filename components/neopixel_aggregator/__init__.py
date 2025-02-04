import esphome.codegen as cg
import esphome.config_validation as cv
from esphome.components import light
from esphome.const import CONF_ID

CODEOWNERS = ["@hinathan"]
DEPENDENCIES = ["api"]

CONF_NEOPIXEL_ID = "neopixel_id"
CONF_LEDS = "leds"
CONF_COLOR = "color"
CONF_ON_COLOR = "on_color"
CONF_OFF_COLOR = "off_color"
CONF_HEARTBEAT = "heartbeat"
CONF_BRIGHTNESS = "brightness"

# Define component namespace
namespace = cg.esphome_ns.namespace("neopixel_aggregator")
NeopixelAggregatorComponent = namespace.class_("NeopixelAggregatorComponent", cg.Component)

CONFIG_SCHEMA = cv.Schema(
    {
        cv.Required(CONF_ID): cv.declare_id(NeopixelAggregatorComponent),
        cv.Required(CONF_NEOPIXEL_ID): cv.use_id(light.AddressableLightState),
        cv.Required(CONF_LEDS): cv.Schema(
            {
                cv.positive_int: cv.All(
                    cv.ensure_list(
                        cv.Any(
                            cv.string,  # Entity ID
                            cv.Schema({cv.Required(cv.string): cv.Schema({cv.Optional(CONF_COLOR, default="#FFFFFF"): cv.string})})  # Entity ID with color
                        )
                    )
                )
            }
        ),
        cv.Optional(CONF_HEARTBEAT, default=-1): cv.int_,
        cv.Optional(CONF_BRIGHTNESS, default=1.0): cv.zero_to_one_float,
        cv.Optional(CONF_ON_COLOR, default="#FFFFFF"): cv.string,
        cv.Optional(CONF_OFF_COLOR, default="#000000"): cv.string,        
    }
).extend(cv.COMPONENT_SCHEMA)

async def to_code(config):
    var = cg.new_Pvariable(config[CONF_ID])
    await cg.register_component(var, config)

    entity_ref = await cg.get_variable(config[CONF_NEOPIXEL_ID])
    if entity_ref is not None:
      cg.add(var.register_entity(entity_ref))

    cg.add(var.register_heartbeat(config[CONF_HEARTBEAT]))
    cg.add(var.set_brightness(config[CONF_BRIGHTNESS]))
    cg.add(var.set_off_color(config[CONF_OFF_COLOR]))

    for led_num, entities in config[CONF_LEDS].items():
        for entity in entities:
            if isinstance(entity, str):
                entity_id = entity
                color = config[CONF_ON_COLOR]
            else:
                entity_id = list(entity.keys())[0]
                color = entity[entity_id].get(CONF_COLOR, config[CONF_ON_COLOR])
            
            cg.add(var.add_led_mapping(led_num, entity_id, color))
