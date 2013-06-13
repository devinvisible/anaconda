from chowdren.writers.objects import ObjectWriter

from chowdren.common import (get_image_name, get_animation_name, to_c,
    make_color)

from chowdren.writers.events import (ComparisonWriter, ActionMethodWriter, 
    ConditionMethodWriter, ExpressionMethodWriter, make_table)

class StringTokenizer(ObjectWriter):
    class_name = 'StringTokenizer'

actions = make_table(ActionMethodWriter, {
    0 : 'split'
})

conditions = make_table(ConditionMethodWriter, {
})

expressions = make_table(ExpressionMethodWriter, {
    1 : 'get'
})

def get_object():
    return StringTokenizer