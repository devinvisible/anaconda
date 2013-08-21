#include "common.h"
#include "movement.h"
#include "filecommon.h"

#include <string>

std::string newline_character("\r\n");

// Font

Font::Font(char * face, int size, bool bold, bool italic, bool underline)
: face(face),  size(size),  bold(bold), italic(italic), underline(underline)
{

}

// Background

Background::Background()
: image(NULL), image_changed(true)
{
    glGenTextures(1, &tex);
    glBindTexture(GL_TEXTURE_2D, tex);
    glTexParameteri(GL_TEXTURE_2D, GL_TEXTURE_MAG_FILTER, GL_LINEAR);
    glTexParameteri(GL_TEXTURE_2D, GL_TEXTURE_MIN_FILTER, GL_LINEAR);
    glTexImage2D(GL_TEXTURE_2D, 0, GL_RGBA, BACK_WIDTH,
        BACK_HEIGHT, 0, GL_RGBA, GL_UNSIGNED_BYTE,
        0);
    reset();
}

Background::~Background()
{
    delete[] image;
    glDeleteTextures(1, &tex);
}

void Background::reset(bool clear_items)
{
    if (image != NULL) {
        delete[] image;
    }
    image = new unsigned char[BACK_WIDTH * BACK_HEIGHT * 4]();
    image_changed = true;
    items_changed = false;
    if (clear_items)
        items.clear();
}

void Background::destroy_at(int x, int y)
{
    std::vector<BackgroundItem>::iterator it = items.begin();
    while (it != items.end()) {
        BackgroundItem & item = (*it);
        if (collides(item.dest_x, item.dest_y, 
                     item.dest_x + item.src_width, 
                     item.dest_y + item.src_height,
                     x, y, x, y)) {
            it = items.erase(it);
            items_changed = true;
        } else
            ++it;
    }
}

void Background::update()
{
    if (!items_changed)
        return;
    reset(false);
    std::vector<BackgroundItem>::const_iterator it;
    for (it = items.begin(); it != items.end(); it++) {
        const BackgroundItem & item = (*it);
        paste(item.image, item.dest_x, item.dest_y, 
              item.src_x, item.src_y, item.src_width, item.src_height,
              item.collision_type, false);
    }
}

void Background::paste(Image * img, int dest_x, int dest_y, 
           int src_x, int src_y, int src_width, int src_height, 
           int collision_type, bool save)
{
    if (save) {
        items.push_back(BackgroundItem(
            img, dest_x, dest_y, src_x, src_y, src_width, src_height, 
            collision_type));
        items_changed = true;
        return;
    }

    if (collision_type == 1)
        return;

    int x, y, dest_x2, dest_y2, src_x2, src_y2;

    for (x = 0; x < src_width; x++) {
        src_x2 = src_x + x;
        if (src_x2 < 0 || src_x2 >= img->width)
            continue;
        dest_x2 = dest_x + x;
        if (dest_x2 < 0 || dest_x2 >= BACK_WIDTH)
            continue;
        for (y = 0; y < src_height; y++) {
            src_y2 = src_y + y;
            if (src_y2 < 0 || src_y2 >= img->height)
                continue;
            dest_y2 = dest_y + y;
            if (dest_y2 < 0 || dest_y2 >= BACK_HEIGHT)
                continue;
            unsigned char * src_c = (unsigned char*)&img->get(
                src_x2, src_y2);
            unsigned char & src_a = src_c[3];

            unsigned char & src_r = src_c[0];
            unsigned char & src_g = src_c[1];
            unsigned char & src_b = src_c[2];
            unsigned char * dst_c = (unsigned char*)&get(dest_x2, 
                dest_y2);
            unsigned char & dst_r = dst_c[0];
            unsigned char & dst_g = dst_c[1];
            unsigned char & dst_b = dst_c[2];
            unsigned char & dst_a = dst_c[3];
            float srcf_a = src_a / 255.0f;
            float one_minus_src = 1.0f - srcf_a;
            dst_r = srcf_a * src_r + one_minus_src * dst_r;
            dst_g = srcf_a * src_g + one_minus_src * dst_g;
            dst_b = srcf_a * src_b + one_minus_src * dst_b;
            dst_a = (srcf_a + (dst_a / 255.0f) * one_minus_src) * 255;
        }
    }
    image_changed = true;
}

void Background::draw()
{
    update();
    glBlendFunc(GL_ONE, GL_ONE_MINUS_SRC_ALPHA);
    glEnable(GL_TEXTURE_2D);
    glBindTexture(GL_TEXTURE_2D, tex);
    glColor4f(1.0, 1.0, 1.0, 1.0);
    if (image_changed) {
        glTexSubImage2D(GL_TEXTURE_2D, 0, 0, 0, BACK_WIDTH,
            BACK_HEIGHT, GL_RGBA, GL_UNSIGNED_BYTE,
            image);
        image_changed = false;
    }
    glBegin(GL_QUADS);
    glTexCoord2f(0.0, 0.0);
    glVertex2d(0, 0);
    glTexCoord2f(1.0, 0.0);
    glVertex2d(BACK_WIDTH, 0.0);
    glTexCoord2f(1.0, 1.0);
    glVertex2d(BACK_WIDTH, BACK_HEIGHT);
    glTexCoord2f(0.0, 1.0);
    glVertex2d(0, BACK_HEIGHT);
    glEnd();
    glDisable(GL_TEXTURE_2D);
    glBlendFunc(GL_SRC_ALPHA, GL_ONE_MINUS_SRC_ALPHA);
}

bool Background::collide(CollisionBase * a)
{
    std::vector<BackgroundItem>::reverse_iterator it;
    for (it = items.rbegin(); it != items.rend(); it++) {
        BackgroundItem & item = *it;
        if (item.collision_type != 1)
            continue;
        if (::collide(a, &item))
            return true;
    }
    return false;
}

// Layer

Layer::Layer(double scroll_x, double scroll_y, bool visible, int index) 
: visible(visible), scroll_x(scroll_x), scroll_y(scroll_y), back(NULL),
  index(index)
{

}

Layer::~Layer()
{
    delete back;

    // layers are in charge of deleting background instances
    for (ObjectList::const_iterator iter = background_instances.begin(); 
         iter != background_instances.end(); iter++) {
        delete (*iter);
    }
}

void Layer::scroll(int dx, int dy)
{
    ObjectList::const_iterator it;
    for (it = instances.begin(); it != instances.end(); it++) {
        FrameObject * object = *it;
        if (object->scroll)
            continue;
        object->set_position(object->x + dx, object->y + dy);
    }
}

void Layer::add_background_object(FrameObject * instance)
{
    background_instances.push_back(instance);
}

void Layer::add_object(FrameObject * instance)
{
    instances.push_back(instance);
}

void Layer::insert_object(FrameObject * instance, int index)
{
    instances.insert(instances.begin() + index, instance);
}

void Layer::remove_object(FrameObject * instance)
{
    instances.erase(
        std::remove(instances.begin(), instances.end(), instance), 
        instances.end());
}

void Layer::set_level(FrameObject * instance, int index)
{
    remove_object(instance);
    if (index == -1)
        add_object(instance);
    else
        insert_object(instance, index);
}

int Layer::get_level(FrameObject * instance)
{
    return std::find(instances.begin(), instances.end(), 
        instance) - instances.begin();
}

void Layer::create_background()
{
    if (back == NULL)
        back = new Background;
}

void Layer::destroy_backgrounds()
{
    if (back == NULL)
        return;
    back->reset();
}

void Layer::destroy_backgrounds(int x, int y, bool fine)
{
    if (fine)
        std::cout << "Destroy backgrounds at " << x << ", " << y <<
            " (" << fine << ") not implemented" << std::endl;
    back->destroy_at(x, y);
}

bool Layer::test_background_collision(int x, int y)
{
    if (back == NULL)
        return false;
    PointCollision col(x, y);
    return back->collide(&col);
}

void Layer::paste(Image * img, int dest_x, int dest_y, 
           int src_x, int src_y, int src_width, int src_height, 
           int collision_type)
{
    create_background();
    back->paste(img, dest_x, dest_y, src_x, src_y, 
        src_width, src_height, collision_type);
}

void Layer::draw()
{ 
    if (!visible)
        return;

    // draw backgrounds
    for (ObjectList::const_iterator iter = background_instances.begin(); 
         iter != background_instances.end(); iter++) {
        FrameObject * item = (*iter);
        if (!item->visible)
            continue;
        item->draw();
    }

    // draw pasted items
    if (back != NULL)
        back->draw();

    // draw active instances
    for (ObjectList::const_iterator iter = instances.begin(); 
         iter != instances.end(); iter++) {
        FrameObject * item = (*iter);
        if (!item->visible)
            continue;
        item->draw();
    }
}

// Frame

Frame::Frame(const std::string & name, int width, int height, 
             Color background_color, int index, GameManager * manager)
: name(name), width(width), height(height), index(index), 
  background_color(background_color), manager(manager),
  off_x(0), off_y(0), has_quit(false), last_key(-1), next_frame(-1),
  loop_count(0), frame_time(0.0), frame_iteration(0)
{
    std::fill_n(key_presses, CHOWDREN_KEY_LAST, false);
    std::fill_n(mouse_presses, CHOWDREN_MOUSE_BUTTON_LAST, false);
}

void Frame::on_start()
{
}

void Frame::on_end() 
{
    ObjectList::const_iterator it;
    for (it = instances.begin(); it != instances.end(); it++) {
        delete (*it);
    }
    std::vector<Layer*>::const_iterator layer_it;
    for (layer_it = layers.begin(); layer_it != layers.end(); layer_it++) {
        delete (*layer_it);
    }
    instances.clear();
    layers.clear();
    instance_classes.clear();
    next_frame = -1;
    loop_count = 0;
    off_x = 0;
    off_y = 0;
    frame_time = 0.0;
    frame_iteration++;
}

void Frame::handle_events() {}

bool Frame::update(float dt)
{
    frame_time += dt;

    for (ObjectList::const_iterator it = instances.begin(); 
         it != instances.end(); it++) {
        FrameObject * instance = *it;
        instance->update(dt);
        if (instance->movement)
            instance->movement->update(dt);
    }

    handle_events();

    for (ObjectList::const_iterator it = destroyed_instances.begin(); 
         it != destroyed_instances.end(); it++) {
        FrameObject * instance = *it;
        instances.erase(
            std::remove(instances.begin(), instances.end(), instance), 
            instances.end());
        ObjectList & class_instances = instance_classes[instance->id];
        class_instances.erase(
            std::remove(class_instances.begin(), class_instances.end(), 
            instance), class_instances.end());
        layers[instance->layer_index]->remove_object(instance);
        delete instance;
    }

    destroyed_instances.clear();

    last_key = -1;

    std::fill_n(key_presses, CHOWDREN_KEY_LAST, false);
    std::fill_n(key_releases, CHOWDREN_KEY_LAST, false);
    std::fill_n(mouse_presses, CHOWDREN_MOUSE_BUTTON_LAST, false);

    loop_count++;

    return !has_quit;
}

void Frame::pause()
{

}

void Frame::restart()
{
    next_frame = -2;
}

void Frame::on_key(int key, bool state)
{
    if (state) {
        last_key = key;
        key_presses[key] = true;
    } else {
        key_releases[key] = true;
    }
}

void Frame::on_mouse(int key, bool state)
{
    if (state) {
        mouse_presses[key] = true;
    }
}

void Frame::draw()
{
    // first, draw the actual window contents
    glViewport(0, 0, WINDOW_WIDTH, WINDOW_HEIGHT);
    glMatrixMode(GL_PROJECTION);
    glLoadIdentity();
    glOrtho(0, WINDOW_WIDTH, WINDOW_HEIGHT, 0, -1, 1);
    glMatrixMode(GL_MODELVIEW);
    glClearColor(background_color.r / 255.0f, background_color.g / 255.0f,
                 background_color.b / 255.0f, 1.0f);
    glClear(GL_COLOR_BUFFER_BIT);

    int index = 0;
    for (std::vector<Layer*>::const_iterator iter = layers.begin(); 
         iter != layers.end(); iter++) {
        glLoadIdentity();
        glTranslatef(floor(-off_x * (*iter)->scroll_x),
                     floor(-off_y * (*iter)->scroll_y), 0.0);
        (*iter)->draw();
        index++;
    }
}

ObjectList & Frame::get_instances(int object_id)
{
    return instance_classes[object_id];
}

ObjectList Frame::get_instances(unsigned int qualifier[])
{
    ObjectList list;
    int index = 0;
    while (qualifier[index] != 0) {
        ObjectList & b = get_instances(qualifier[index]);
        list.insert(list.end(), b.begin(), b.end());
        index++;
    }
    return list;
}

FrameObject * Frame::get_instance(int object_id)
{
    ObjectList & instances = instance_classes[object_id];
    if (instances.size() == 0)
        std::cout << "Fatal error: invalid instance count" << std::endl;
    return instances[0];
}

FrameObject * Frame::get_instance(unsigned int qualifier[])
{
    ObjectList list = get_instances(qualifier);
    if (list.size() == 0)
        std::cout << "Fatal error: invalid instance count" << std::endl;
    return list[0];
}

void Frame::add_layer(double scroll_x, double scroll_y, bool visible)
{
    layers.push_back(new Layer(scroll_x, scroll_y, visible, layers.size()));
}

void Frame::add_object(FrameObject * object, int layer_index)
{
    layer_index = int_max(0, int_min(layer_index, layers.size() - 1));
    object->frame = this;
    object->layer_index = layer_index;
    instances.push_back(object);
    instance_classes[object->id].push_back(object);
    layers[layer_index]->add_object(object);
}

void Frame::add_background_object(FrameObject * object, int layer_index)
{
    object->frame = this;
    object->layer_index = layer_index;
    layers[layer_index]->add_background_object(object);
}

ObjectList Frame::create_object(FrameObject * object, int layer_index)
{
    add_object(object, layer_index);
    ObjectList new_list;
    new_list.push_back(object);
    return new_list;
}

void Frame::destroy_object(FrameObject * object)
{
    if (object->destroying)
        return;
    object->destroying = true;
    destroyed_instances.push_back(object);
}

void Frame::set_object_layer(FrameObject * object, int new_layer)
{
    layers[object->layer_index]->remove_object(object);
    layers[new_layer]->add_object(object);
    object->layer_index = new_layer;
}

int Frame::get_loop_index(const std::string & name)
{
    return loop_indexes[name];
}

void Frame::set_timer(double value)
{
    frame_time = value;
}

void Frame::set_display_center(int x, int y)
{
    int old_off_x, old_off_y;
    old_off_x = off_x;
    old_off_y = off_y;

    if (x != -1) 
        off_x = x - WINDOW_WIDTH / 2;

    if (y != -1)
        off_y = int_min(
            int_max(0, y - WINDOW_HEIGHT / 2),
            height - WINDOW_HEIGHT);

    int dx, dy;
    dx = off_x - old_off_x;
    dy = off_y - old_off_y;

    std::vector<Layer*>::const_iterator it;
    for (it = layers.begin(); it != layers.end(); it++) {
        Layer * layer = *it;
        layer->scroll(dx, dy);
    }
}

int Frame::frame_right()
{
    return off_x + WINDOW_WIDTH;
}

int Frame::frame_bottom()
{
    return off_y + WINDOW_HEIGHT;
}

void Frame::set_background_color(int color)
{
    background_color = Color(color);
}

void Frame::get_mouse_pos(int * x, int * y)
{
    (*x) = manager->mouse_x + off_x;
    (*y) = manager->mouse_y + off_y;
}

int Frame::get_mouse_x()
{
    int x, y;
    get_mouse_pos(&x, &y);
    return x;
}

int Frame::get_mouse_y()
{
    int x, y;
    get_mouse_pos(&x, &y);
    return y;
}

bool Frame::is_mouse_pressed_once(int key)
{
    if (key < 0)
        return false;
    return (bool)mouse_presses[key];
}

bool Frame::is_key_released_once(int key)
{
    if (key < 0)
        return false;
    return (bool)key_releases[key];
}

bool Frame::is_key_pressed_once(int key)
{
    if (key < 0)
        return false;
    return (bool)key_presses[key];
}

bool Frame::test_background_collision(int x, int y)
{
    if (x < 0 || y < 0 || x > width || y > width)
        return false;
    std::vector<Layer*>::const_iterator it;
    for (it = layers.begin(); it != layers.end(); it++) {
        if ((*it)->test_background_collision(x, y))
            return true;
    }
    return false;
}

bool Frame::compare_joystick_direction(int n, int test_dir)
{
    return get_joystick_direction(n) == test_dir;
}

bool Frame::is_joystick_direction_changed(int n)
{
    // hack for now
    static int last_dir = get_joystick_direction(n);
    int new_dir = get_joystick_direction(n);
    bool ret = last_dir != new_dir;
    last_dir = new_dir;
    return ret;
}

// FrameObject

FrameObject::FrameObject(const std::string & name, int x, int y, int type_id) 
: x(x), y(y), id(type_id), visible(true), shader(NULL), 
  values(NULL), strings(NULL), shader_parameters(NULL), direction(0), 
  destroying(false), scroll(true), name(name), movement(NULL)
{
}

FrameObject::~FrameObject()
{
    delete movement;
    delete values;
    delete strings;
    delete shader_parameters;
}

void FrameObject::draw_image(Image * img, double x, double y, double angle,
                             double x_scale, double y_scale, bool flip_x,
                             bool flip_y)
{
    GLuint back_tex = 0;
    if (shader != NULL) {
        shader->begin(this, img);
        back_tex = shader->get_background_texture();
    }
    img->draw(x, y, angle, x_scale, y_scale, flip_x, flip_y, back_tex);
    if (shader != NULL)
        shader->end(this);
}

void FrameObject::set_position(int x, int y)
{
    this->x = x;
    this->y = y;
}

void FrameObject::set_x(int x)
{
    this->x = x;
}

int FrameObject::get_x()
{
    Layer * layer = frame->layers[layer_index];
    return x + frame->off_x * (1 - layer->scroll_x);
}

void FrameObject::set_y(int y)
{
    this->y = y;
}

int FrameObject::get_y()
{
    Layer * layer = frame->layers[layer_index];
    return y + frame->off_y * (1 - layer->scroll_y);
}

void FrameObject::create_alterables()
{
    values = new AlterableValues;
    strings = new AlterableStrings;
}

void FrameObject::set_visible(bool value)
{
    visible = value;
}

void FrameObject::set_blend_color(int color)
{
    int a = blend_color.a;
    blend_color = Color(color);
    blend_color.a = a;
}

void FrameObject::set_layer(int index)
{
    frame->set_object_layer(this, index);
}

void FrameObject::draw() {}
void FrameObject::update(float dt) {}
void FrameObject::set_direction(int value)
{
    direction = value & 31;
}

int FrameObject::get_direction() 
{
    return direction;
}

CollisionBase * FrameObject::get_collision()
{
    return NULL;
}

bool FrameObject::mouse_over()
{
    if (destroying)
        return false;
    int x, y;
    frame->get_mouse_pos(&x, &y);
    PointCollision col1(x, y);
    return collide(&col1, get_collision());
}

bool FrameObject::overlaps(FrameObject * other)
{
    if (destroying || other->destroying)
        return false;
    return collide(other->get_collision(), get_collision());
}

bool FrameObject::overlaps_background()
{
    if (destroying)
        return false;
    Background * back = frame->layers[layer_index]->back;
    back->update();
    return back->collide(get_collision());
}

bool FrameObject::outside_playfield()
{
    int box[4];
    get_collision()->get_box(box);
    return !collides(box[0], box[1], box[2], box[3],
        0, 0, frame->width, frame->height);
}

int FrameObject::get_box_index(int index)
{
    int box[4];
    get_collision()->get_box(box);
    return box[index];
}

void FrameObject::get_box(int box[4])
{
    get_collision()->get_box(box);
}

void FrameObject::set_shader(Shader * value)
{
    if (shader_parameters == NULL)
        shader_parameters = new ShaderParameters;
    shader = value;
}

void FrameObject::set_shader_parameter(const std::string & name, double value)
{
    if (shader_parameters == NULL)
        shader_parameters = new ShaderParameters;
    (*shader_parameters)[name] = value;
}

void FrameObject::set_shader_parameter(const std::string & name,
                                       const Color & color)
{
    set_shader_parameter(name, (double)color.get_int());
}

double FrameObject::get_shader_parameter(const std::string & name)
{
    if (shader_parameters == NULL)
        return 0.0;
    return (*shader_parameters)[name];
}

void FrameObject::destroy()
{
    frame->destroy_object(this);
}

void FrameObject::set_level(int index)
{
    frame->layers[layer_index]->set_level(this, index);
}

int FrameObject::get_level()
{
    return frame->layers[layer_index]->get_level(this);
}

void FrameObject::move_back(FrameObject * other)
{
    set_level(other->get_level()+1);
}

void FrameObject::move_back()
{
    set_level(0);
}

void FrameObject::move_front()
{
    set_level(-1);
}

void FrameObject::move_front(FrameObject * other)
{
    set_level(other->get_level());
}

FixedValue FrameObject::get_fixed()
{
    return FixedValue(this);
}

void FrameObject::set_movement(int i)
{
    
}

Movement * FrameObject::get_movement()
{
    if (movement == NULL)
        movement = new StaticMovement(this);
    return movement;
}

// FixedValue

FixedValue::FixedValue(FrameObject * object)
: object(object)
{

}

FixedValue::operator double() const
{
    int64_t v = int64_t(intptr_t(object));
    double v2;
    memcpy(&v2, &v, sizeof(double));
    return v2;
}

FixedValue::operator std::string() const
{
    intptr_t val = intptr_t(object);
    return number_to_string(val);
}

// Direction

Direction::Direction(int index, int min_speed, int max_speed, int loop_count, 
          int back_to)
: index(index), min_speed(min_speed), max_speed(max_speed), 
  loop_count(loop_count), back_to(back_to)
{

}

Direction::Direction()
{
    
}

// DirectionArray

DirectionArrayStruct::DirectionArrayStruct()
{
    for (int i = 0; i < 32; i++) {
        dirs[i] = NULL;
    }
}

// Active

inline int direction_diff(int dir1, int dir2)
{
    return (((dir1 - dir2 + 540) % 360) - 180);
}

inline Direction * find_nearest_direction(int dir, DirectionArray & dirs)
{
    int i = 0;
    Direction * check_dir;
    while (true) {
        i++;
        check_dir = dirs[(dir + i) & 31];
        if (check_dir != NULL)
            return check_dir;
        check_dir = dirs[(dir - i) & 31];
        if (check_dir != NULL)
            return check_dir;
    }
}

Active::Active(const std::string & name, int x, int y, int type_id) 
: FrameObject(name, x, y, type_id), animation(),
  animation_frame(0), counter(0), angle(0.0), forced_frame(-1), 
  forced_speed(-1), forced_direction(-1), x_scale(1.0), y_scale(1.0),
  animation_direction(0), stopped(false), flash_interval(0.0f)
{
    create_alterables();
}

void Active::initialize_active()
{
    collision = new SpriteCollision(this, NULL);
    collision->is_box = collision_box;
    update_frame();
}

Active::~Active()
{
    delete collision;
}

void Active::initialize_animations()
{
    AnimationMap::iterator it;
    for (it = animations->begin(); it != animations->end(); it++) {
        DirectionArray & current_dirs = it->second.dirs;
        DirectionArray check_dirs;
        memcpy(check_dirs, current_dirs, sizeof(DirectionArray));
        for (int i = 0; i < 32; i++) {
            if (current_dirs[i] == NULL)
                current_dirs[i] = find_nearest_direction(i, check_dirs);
        }
    }
}

void Active::force_animation(int value)
{
    if (value == animation)
        return;
    if (animations->find(value) == animations->end())
        std::cout << "Invalid animation: " << value 
            << " (" << name << ")" << std::endl;
    animation = value;
    if (forced_frame == -1)
        animation_frame = 0;
    update_frame();
}

void Active::force_frame(int value)
{
    forced_frame = value;
    update_frame();
}

void Active::force_speed(int value)
{
    forced_speed = value;
}

void Active::force_direction(int value)
{
    forced_direction = value & 31;
    update_frame();
}

void Active::restore_animation()
{

}

void Active::restore_frame()
{
    animation_frame = forced_frame;
    forced_frame = -1;
    update_frame();
}

void Active::restore_speed()
{
    forced_speed = -1;
}

void Active::add_direction(int animation, int direction,
                   int min_speed, int max_speed, int loop_count,
                   int back_to)
{
    (*animations)[animation].dirs[direction] = new Direction(direction, 
        min_speed, max_speed, loop_count, back_to);
}

void Active::add_image(int animation, int direction, Image * image)
{
    (*animations)[animation].dirs[direction]->frames.push_back(image);
}

void Active::update_frame()
{
    int frame_count = get_direction_data()->frames.size();
    int & current_frame = get_frame();
    current_frame = int_max(0, int_min(current_frame, frame_count - 1));

    Image * img = get_image();
    if (img == NULL)
        return;

    collision->set_image(img);
    update_action_point();
}

void Active::update_action_point()
{
    Image * img = get_image();
    collision->get_transform(img->action_x, img->action_y, 
                             action_x, action_y);
    action_x -= collision->hotspot_x;
    action_y -= collision->hotspot_y;
}

void Active::update(float dt)
{
    if (forced_frame != -1 || stopped)
        return;
    if (flash_interval != 0.0f) {
        flash_time += dt;
        if (flash_time >= flash_interval) {
            flash_time = 0.0f;
            visible = !visible;
        }
    }
    Direction * dir = get_direction_data();
    counter += get_speed();
    int old_frame = animation_frame;
    while (counter > 100) {
        animation_frame++;
        if (animation_frame >= (int)dir->frames.size()) {
            if (dir->loop_count != 0)
                animation_frame--;
            else
                animation_frame = dir->back_to;
        }
        counter -= 100;
    }
    if (animation_frame != old_frame)
        update_frame();
}

void Active::draw()
{
    Image * img = get_image();
    if (img == NULL) {
        std::cout << "Invalid image draw (" << name << ")" << std::endl;
        return;
    }
    blend_color.apply();
    draw_image(img, x, y, angle, x_scale, y_scale, false, false);
}

inline Image * Active::get_image()
{
    AnimationMap::const_iterator it = animations->find(animation);
    if (it == animations->end()) {
        std::cout << "Invalid animation: " << animation << std::endl;
        return NULL;
    }
    const DirectionArray & dirs = it->second.dirs;
    const Direction * dir = dirs[get_animation_direction()];
    if (get_frame() >= (int)dir->frames.size()) {
        std::cout << "Invalid frame: " << get_frame() << " " <<
            dir->frames.size() << " " <<
            "(" << name << ")" << std::endl;
        return NULL;
    }
    return dir->frames[get_frame()];
}

int Active::get_action_x()
{
    return x + action_x;
}

int Active::get_action_y()
{
    return y + action_y;
}

void Active::set_angle(double angle, int quality)
{
    angle = mod(angle, 360.0f);
    this->angle = angle;
    collision->set_angle(angle);
    update_action_point();
}

double Active::get_angle()
{
    return angle;
}

int & Active::get_frame()
{
    if (forced_frame != -1)
        return forced_frame;
    return animation_frame;
}

int Active::get_speed()
{
    if (forced_speed != -1)
        return forced_speed;
    return get_direction_data()->max_speed;
}

Direction * Active::get_direction_data(int & dir)
{
    AnimationMap & ref = *animations;
    DirectionArray & array = ref[get_animation()].dirs;
    return array[dir];
}

Direction * Active::get_direction_data()
{
    return get_direction_data(get_animation_direction());
}

int Active::get_animation()
{
    return animation;
}

CollisionBase * Active::get_collision()
{
    return collision;
}

void Active::set_direction(int value)
{
    FrameObject::set_direction(value);
    animation_direction = direction;
    update_frame();
}

int & Active::get_animation_direction()
{
    if (forced_direction != -1)
        return forced_direction;
    return animation_direction;
}

void Active::set_scale(double scale)
{
    x_scale = y_scale = scale;
    collision->set_scale(scale);
    update_action_point();
}

void Active::set_x_scale(double value)
{
    x_scale = value;
    collision->set_x_scale(value);
    update_action_point();
}

void Active::set_y_scale(double value)
{
    y_scale = value;
    collision->set_y_scale(value);
    update_action_point();
}

void Active::paste(int collision_type)
{
    Image * img = get_image();
    frame->layers[layer_index]->paste(img, 
        x-img->hotspot_x, y-img->hotspot_y, 0, 0, 
        img->width, img->height, collision_type);
}

bool Active::test_direction(int value)
{
    return get_direction() == value;
}

bool Active::test_directions(int value)
{
    int direction = get_direction();
    return ((value >> direction) & 1) != 0;
}

bool Active::test_animation(int value)
{
    return value == animation;
}

void Active::stop_animation()
{
    stopped = true;
}

void Active::start_animation()
{
    stopped = false;
}

void Active::flash(float value)
{
    flash_interval = value;
    flash_time = 0.0f;
}

// Text

/*
static FTTextureFont * default_font = NULL;*/

void set_font_path(const char * path)
{
/*    if (default_font != NULL)
        return;
    default_font = new FTTextureFont(path, false);*/
}

void set_font_path(const std::string & path)
{
    set_font_path(path.c_str());
}

void init_font()
{
    static bool initialized = false;
    if (initialized)
        return;
    set_font_path("Arial.ttf"); // default font, could be set already
/*    default_font->FaceSize(12, 96);*/
    initialized = true;
}


Text::Text(const std::string & name, int x, int y, int type_id) 
: FrameObject(name, x, y, type_id), initialized(false), current_paragraph(0)
{
    create_alterables();
    collision = new InstanceBox(this);
}

Text::~Text()
{
    delete collision;
}

void Text::add_line(std::string text)
{
    paragraphs.push_back(text);
    if (!initialized) {
        initialized = true;
        this->text = text;
    }
}

void Text::draw()
{
/*        init_font();
    color.apply();
    glPushMatrix();
    FTBBox box = default_font->BBox(text.c_str(), text.size(), FTPoint());
    double box_w = box.Upper().X() - box.Lower().X();
    double box_h = box.Upper().Y() - box.Lower().Y();
    double off_x = x;
    double off_y = y + default_font->Ascender();

    if (alignment & ALIGN_HCENTER)
        off_x += 0.5 * (width - box_w);
    else if (alignment & ALIGN_RIGHT) 
        off_x += width - box_w;

    if (alignment & ALIGN_VCENTER)
        off_y += height * 0.5 - default_font->LineHeight() * 0.5;
    else if (alignment & ALIGN_BOTTOM)
        off_y += default_font->LineHeight();

    glTranslated((int)off_x, (int)off_y, 0.0);
    glScalef(1, -1, 1);
    default_font->Render(text.c_str(), text.size(), FTPoint(),
        FTPoint(), RENDER_ALL);
    glPopMatrix();*/
}

void Text::set_string(std::string value)
{
    text = value;
}

void Text::set_paragraph(unsigned int index)
{
    current_paragraph = index;
    set_string(get_paragraph(index));
}

void Text::next_paragraph()
{
    set_paragraph(current_paragraph + 1);
}

int Text::get_index()
{
    return current_paragraph;
}

int Text::get_count()
{
    return paragraphs.size();
}

bool Text::get_bold()
{
    return bold;
}

bool Text::get_italic()
{
    return italic;
}

void Text::set_bold(bool value)
{
    bold = value;
}

std::string Text::get_paragraph(int index)
{
    if (index < 0)
        index = 0;
    else if (index >= (int)paragraphs.size())
        index = paragraphs.size() - 1;
    return paragraphs[index];
}

CollisionBase * Text::get_collision()
{
    return collision;
}

// Backdrop

Backdrop::Backdrop(Image * image, const std::string & name, int x, int y,
                   int type_id) 
: FrameObject(name, x, y, type_id), image(image)
{
}

void Backdrop::draw()
{
    glColor4f(1.0f, 1.0f, 1.0f, 1.0f);
    int hotspot_x = image->hotspot_x;
    int hotspot_y = image->hotspot_y;
    image->hotspot_x = image->hotspot_y = 0;
    image->draw(x, y);
    image->hotspot_x = hotspot_x;
    image->hotspot_y = hotspot_y;
}

// QuickBackdrop

QuickBackdrop::QuickBackdrop(const Color & color, int width, int height, 
              const std::string & name, int x, int y, int type_id) 
: FrameObject(name, x, y, type_id), color(color)
{
    this->width = width;
    this->height = height;
}

void QuickBackdrop::draw()
{
    glDisable(GL_TEXTURE_2D);
    glColor4ub(color.r, color.g, color.b, blend_color.a);
    glBegin(GL_QUADS);
    glVertex2f(x, y);
    glVertex2f(x + width, y);
    glVertex2f(x + width, y + height);
    glVertex2f(x, y + height);
    glEnd();
}

// Counter

Counter::Counter(int init, int min, int max, const std::string & name, 
                 int x, int y, int type_id) 
: FrameObject(name, x, y, type_id), minimum(min), maximum(max)
{
    for (int i = 0; i < 14; i++)
        images[i] = NULL;

    set(init);
}

Image * Counter::get_image(char c)
{
    switch (c) {
        case '0':
        case '1':
        case '2':
        case '3':
        case '4':
        case '5':
        case '6':
        case '7':
        case '8':
        case '9':
            return images[c - '0'];
        case '-':
            return images[10];
        case '+':
            return images[11];
        case '.':
            return images[12];
        case 'e':
            return images[13];
        default:
            return NULL;
    }
}

void Counter::add(double value)
{
    set(this->value + value);
}

void Counter::set(double value)
{
    value = std::max<double>(std::min<double>(value, maximum), minimum);
    this->value = value;

    std::ostringstream str;
    str << value;
    cached_string = str.str();
}

void Counter::draw()
{
    glColor4f(1.0f, 1.0f, 1.0f, 1.0f);
    double current_x = x;
    for (std::string::reverse_iterator it = cached_string.rbegin(); 
         it != cached_string.rend(); it++) {
        Image * image = get_image(it[0]);
        if (image == NULL)
            continue;
        image->draw(current_x + image->hotspot_x - image->width, 
                    y + image->hotspot_y - image->height);
        current_x -= image->width;
    }
}

// INI

#include "ini.cpp"

inline bool match_wildcard(const std::string & pattern, 
                           const std::string & value)
{
    if (pattern.empty() || value.empty())
        return pattern == value;
    if (pattern == "*")
        return true;
    else if (pattern[0] == '*') {
        size_t size = pattern.size() - 1;
        if (size > value.size())
            return false;
        return pattern.compare(1, size, value, value.size() - size, size) == 0;
    } else if (pattern[pattern.size() - 1] == '*') {
        size_t size = pattern.size() - 1;
        if (size > value.size())
            return false;
        return pattern.compare(0, size, value, 0, size) == 0;
    } else if (std::count(pattern.begin(), pattern.end(), '*') > 0) {
        std::cout << "Generic wildcard not implemented yet: " << pattern 
            << std::endl;
        return false;
    }
    return value == pattern;
}

INI::INI(const std::string & name, int x, int y, int type_id) 
: FrameObject(name, x, y, type_id), overwrite(false), auto_save(false)
{
    create_alterables();
}

void INI::reset_global_data()
{
    global_data.clear();
}

int INI::_parse_handler(void* user, const char* section, const char* name,
                               const char* value)
{
    INI * reader = (INI*)user;
    reader->parse_handler(section, name, value);
    return 1;
}

void INI::parse_handler(const std::string & section, const std::string & name,
                        const std::string & value)
{
    if (!overwrite && has_item(section, name))
        return;
    data[section][name] = value;
}

void INI::set_group(const std::string & name, bool new_group)
{
    current_group = name;
}

std::string INI::get_string(const std::string & group, const std::string & item, 
                            const std::string & def)
{
    SectionMap::const_iterator it = data.find(group);
    if (it == data.end())
        return def;
    OptionMap::const_iterator new_it = (*it).second.find(item);
    if (new_it == (*it).second.end())
        return def;
    return (*new_it).second;
}

std::string INI::get_string(const std::string & item, const std::string & def)
{
    return get_string(current_group, item, def);
}

std::string INI::get_string_index(const std::string & group, unsigned int index)
{
    SectionMap::const_iterator it = data.find(group);
    if (it == data.end())
        return "";
    OptionMap::const_iterator new_it = (*it).second.begin();
    int current_index = 0;
    while (new_it != (*it).second.end()) {
        if (current_index == index)
            return (*new_it).second;
        new_it++;
        current_index++;
    }
    return "";
}

std::string INI::get_string_index(unsigned int index)
{
    return get_string_index(current_group, index);
}

std::string INI::get_item_name(const std::string & group, unsigned int index)
{
    SectionMap::const_iterator it = data.find(group);
    if (it == data.end())
        return "";
    OptionMap::const_iterator new_it = (*it).second.begin();
    int current_index = 0;
    while (new_it != (*it).second.end()) {
        if (current_index == index)
            return (*new_it).first;
        new_it++;
        current_index++;
    }
    return "";
}

std::string INI::get_item_name(unsigned int index)
{
    return get_item_name(current_group, index);
}

std::string INI::get_group_name(unsigned int index)
{
    SectionMap::const_iterator it = data.begin();
    int current_index = 0;
    while (it != data.end()) {
        if (current_index == index)
            return (*it).first;
        it++;
        current_index++;
    }
    return "";
}

double INI::get_value(const std::string & group, const std::string & item, 
                      double def)
{
    SectionMap::const_iterator it = data.find(group);
    if (it == data.end())
        return def;
    OptionMap::const_iterator new_it = (*it).second.find(item);
    if (new_it == (*it).second.end())
        return def;
    return string_to_double((*new_it).second, def);
}

double INI::get_value(const std::string & item, double def)
{
    return get_value(current_group, item, def);
}

double INI::get_value_index(const std::string & group, unsigned int index)
{
    SectionMap::const_iterator it = data.find(group);
    if (it == data.end())
        return 0.0;
    OptionMap::const_iterator new_it = (*it).second.begin();
    int current_index = 0;
    while (new_it != (*it).second.end()) {
        if (current_index == index)
            return string_to_double((*new_it).second, 0.0);
        new_it++;
        current_index++;
    }
    return 0.0;
}

double INI::get_value_index(unsigned int index)
{
    return get_value_index(current_group, index);
}

void INI::set_value(const std::string & group, const std::string & item, 
               int pad, double value)
{
    set_string(group, item, number_to_string(value));
}

void INI::set_value(const std::string & item, int pad, double value)
{
    set_value(current_group, item, pad, value);
}

void INI::set_string(const std::string & group, const std::string & item, 
                     const std::string & value)
{
    data[group][item] = value;
    save_auto();
}

void INI::set_string(const std::string & item, const std::string & value)
{
    set_string(current_group, item, value);
}

void INI::load_file(const std::string & fn, bool read_only, bool merge,
                    bool overwrite)
{
    this->read_only = read_only;
    filename = convert_path(fn);
    if (!merge)
        reset(false);
    std::cout << "Loading " << filename << " (" << name << ")" << std::endl;
    create_directories(filename);
    int e = ini_parse_file(filename.c_str(), _parse_handler, this);
    std::cout << "Done loading" << std::endl;
    if (e != 0) {
        std::cout << "INI load failed (" << filename << ") with code " << e
        << std::endl;
    }
}

void INI::load_string(const std::string & data, bool merge)
{
    if (!merge)
        reset(false);
    int e = ini_parse_string(data, _parse_handler, this);
    if (e != 0) {
        std::cout << "INI load failed with code " << e << std::endl;
    }
}

void INI::merge_file(const std::string & fn, bool overwrite)
{
    load_file(fn, false, true, overwrite);
}

void INI::get_data(std::stringstream & out)
{
    SectionMap::const_iterator it1;
    OptionMap::const_iterator it2;
    for (it1 = data.begin(); it1 != data.end(); it1++) {
        out << "[" << (*it1).first << "]" << std::endl;
        for (it2 = (*it1).second.begin(); it2 != (*it1).second.end(); 
             it2++) {
            out << (*it2).first << "=" << (*it2).second << std::endl;
        }
        out << std::endl;
    }
}

void INI::save_file(const std::string & fn, bool force)
{
    if (fn.empty() || (read_only && !force))
        return;
    filename = convert_path(fn);
    create_directories(filename);
    std::stringstream out;
    get_data(out);
    FSFile fp(filename.c_str(), "w");
    std::string outs = out.str();
    fp.write(&outs[0], outs.size());
    fp.close();
}

std::string INI::as_string()
{
    std::stringstream out;
    get_data(out);
    return out.str();
}

void INI::save_file(bool force)
{
    save_file(filename, force);
}

void INI::save_auto()
{
    if (!auto_save)
        return;
    save_file(false);
}

int INI::get_item_count(const std::string & section)
{
    return data[section].size();
}

int INI::get_item_count()
{
    return get_item_count(current_group);
}

int INI::get_group_count()
{
    return data.size();
}

bool INI::has_group(const std::string & group)
{
    SectionMap::const_iterator it = data.find(group);
    if (it == data.end())
        return false;
    return true;
}

bool INI::has_item(const std::string & group, const std::string & option)
{
    SectionMap::const_iterator it = data.find(group);
    if (it == data.end())
        return false;
    OptionMap::const_iterator new_it = (*it).second.find(option);
    if (new_it == (*it).second.end())
        return false;
    return true;
}

bool INI::has_item(const std::string & option)
{
    return has_item(current_group, option);
}

void INI::search(const std::string & group, const std::string & item, 
                 const std::string & value)
{
    search_results.clear();
    SectionMap::const_iterator it1;
    OptionMap::const_iterator it2;
    for (it1 = data.begin(); it1 != data.end(); it1++) {
        if (!match_wildcard(group, (*it1).first))
            continue;
        for (it2 = (*it1).second.begin(); it2 != (*it1).second.end(); 
             it2++) {
            if (!match_wildcard(item, (*it2).first))
                continue;
            if (!match_wildcard(value, (*it2).second))
                continue;
            search_results.push_back(
                std::pair<std::string, std::string>(
                    (*it1).first,
                    (*it2).first
                ));
        }
    }
}

void INI::delete_pattern(const std::string & group, const std::string & item, 
                         const std::string & value)
{
    SectionMap::iterator it1;
    OptionMap::iterator it2;
    for (it1 = data.begin(); it1 != data.end(); it1++) {
        if (!match_wildcard(group, (*it1).first))
            continue;
        OptionMap & option_map = (*it1).second;
        it2 = option_map.begin();
        while (it2 != option_map.end()) {
            if (!match_wildcard(item, (*it2).first) || 
                !match_wildcard(value, (*it2).second)) {
                it2++;
                continue;
            }
            option_map.erase(it2++);
        }
    }
    save_auto();
}

void INI::sort_group_by_name(const std::string & group)
{

}

void INI::reset(bool save)
{
    data.clear();
    if (save)
        save_auto();
}

void INI::delete_group(const std::string & group)
{
    data.erase(group);
    save_auto();
}

void INI::delete_group()
{
    delete_group(current_group);
}

void INI::delete_item(const std::string & group, const std::string & item)
{
    data[group].erase(item);
    save_auto();
}

void INI::delete_item(const std::string & item)
{
    delete_item(current_group, item);
}

void INI::set_global_data(const std::string & key)
{
    data = global_data[key];
    global_key = key;
}

void INI::merge_object(INI * other, bool overwrite)
{
    merge_map(other->data, overwrite);
}

void INI::merge_group(INI * other, const std::string & src_group, 
                     const std::string & dst_group, bool overwrite)
{
    merge_map(other->data, src_group, dst_group, overwrite);
}

void INI::merge_map(const SectionMap & data2, bool overwrite)
{
    SectionMap::const_iterator it1;
    OptionMap::const_iterator it2;
    for (it1 = data2.begin(); it1 != data2.end(); it1++) {
        for (it2 = (*it1).second.begin(); it2 != (*it1).second.end(); 
             it2++) {
            if (!overwrite && has_item((*it1).first, (*it2).first))
                continue;
            data[(*it1).first][(*it2).first] = (*it2).second;
        }
    }
    save_auto();
}

void INI::merge_map(SectionMap & data2, const std::string & src_group, 
                    const std::string & dst_group, bool overwrite)
{
    OptionMap & items = data2[src_group];
    OptionMap::const_iterator it;
    for (it = items.begin(); it != items.end(); it++) {
        if (!overwrite && has_item(dst_group, (*it).first))
            continue;
        data[dst_group][(*it).first] = (*it).second;
    }
    save_auto();
}

size_t INI::get_search_count()
{
    return search_results.size();
}

std::string INI::get_search_result_group(int index)
{
    return search_results[index].first;
}

std::string INI::get_item_part(const std::string & group, 
                               const std::string & item, int index,
                               const std::string & def)
{
    if (index < 0)
        return def;
    std::string value = get_string(group, item, "");
    std::vector<std::string> elem;
    split_string(value, ',', elem);
    if (index >= (int)elem.size())
        return def;
    return elem[index];
}

INI::~INI()
{
    if (global_key.empty())
        return;
    global_data[global_key] = data;
}

std::map<std::string, SectionMap> INI::global_data;

// StringTokenizer

StringTokenizer::StringTokenizer(const std::string & name, int x, int y, 
                                 int type_id) 
: FrameObject(name, x, y, type_id)
{
}

void StringTokenizer::split(const std::string & text, 
                            const std::string & delims)
{
    elements.clear();
    split_string(text, delims, elements);
}

const std::string & StringTokenizer::get(int index)
{
    return elements[index];
}

// File

#ifdef _WIN32
#include "windows.h"
#include "shlobj.h"
#elif __APPLE__
#include <CoreServices/CoreServices.h>
#include <limits.h>
#elif __linux
#include <unistd.h>
#include <sys/types.h>
#include <pwd.h>
#endif


std::string File::get_appdata_directory()
{
    static bool initialized = false;
    static std::string dir;
    if (!initialized) {
#ifdef _WIN32
        TCHAR path[MAX_PATH];
        SHGetFolderPath(NULL, CSIDL_APPDATA, NULL, 0, path);
        dir = path;
#elif __APPLE__
        FSRef ref;
        OSType folderType = kApplicationSupportFolderType;
        char path[PATH_MAX];
        FSFindFolder(kUserDomain, folderType, kCreateFolder, &ref);
        FSRefMakePath(&ref, (UInt8*)&path, PATH_MAX);
        dir = path;
#elif __linux
        struct passwd *pw = getpwuid(getuid());
        dir = pw->pw_dir;
#else
        dir = ".";
#endif
    }
    return dir;
}

bool File::file_exists(const std::string & path)
{
    FSFile fp(convert_path(path).c_str(), "r");
    return fp.is_open();
}

void File::delete_file(const std::string & path)
{
    if (!platform_remove_file(path))
        std::cout << "Could not remove " << path << std::endl;
}

// WindowControl

bool WindowControl::has_focus()
{
    return platform_has_focus();
}

void WindowControl::set_focus(bool value)
{
    platform_set_focus(value);
}

// Workspace

Workspace::Workspace(BaseStream & stream)
{
    stream >> name;
    unsigned int len;
    stream >> len;
    stream.read(data, len);
}

Workspace::Workspace(const std::string & name)
: name(name)
{
}

// BinaryArray

BinaryArray::BinaryArray(const std::string & name, int x, int y, int type_id) 
: FrameObject(name, x, y, type_id), current(NULL)
{

}

void BinaryArray::load_workspaces(const std::string & filename)
{
    FSFile fp(convert_path(filename).c_str(), "r");
    FileStream stream(fp);
    Workspace * workspace;
    while (!stream.at_end()) {
        workspace = new Workspace(stream);
        workspaces[workspace->name] = workspace;
    }
    fp.close();
    switch_workspace(current);
}

BinaryArray::~BinaryArray()
{
    WorkspaceMap::const_iterator it;
    for (it = workspaces.begin(); it != workspaces.end(); it++)
        delete it->second;
}

void BinaryArray::create_workspace(const std::string & name)
{
    if (workspaces.find(name) != workspaces.end())
        return;
    Workspace * workspace = new Workspace(name);
    workspaces[name] = workspace;
}

void BinaryArray::switch_workspace(const std::string & name)
{
    WorkspaceMap::const_iterator it = workspaces.find(name);
    if (it == workspaces.end())
        return;
    switch_workspace(it->second);
}

void BinaryArray::switch_workspace(Workspace * workspace)
{
    current = workspace;
}

bool BinaryArray::has_workspace(const std::string & name)
{
    return workspaces.count(name) > 0;
}

void BinaryArray::load_file(const std::string & filename)
{
    size_t size;
    char * data;
    read_file(convert_path(filename).c_str(), &data, &size);
    current->data.write(data, size);
    delete[] data;
}

std::string BinaryArray::read_string(int pos, size_t size)
{
    DataStream stream(current->data);
    stream.seek(pos);
    std::string v;
    stream.read_string(v, size);
    return v;
}

size_t BinaryArray::get_size()
{
    std::stringstream & oss = current->data;
    std::stringstream::pos_type current = oss.tellg();
    oss.seekg(0, std::ios::end);
    std::stringstream::pos_type offset = oss.tellg();
    oss.seekg(current);
    return (size_t)offset;
}

// ArrayObject

ArrayObject::ArrayObject(const std::string & name, int x, int y, int type_id) 
: FrameObject(name, x, y, type_id), array(NULL)
{

}

void ArrayObject::initialize(int x, int y, int z)
{
    x_size = x;
    y_size = y;
    z_size = z;
    clear();
}

double & ArrayObject::get_value(int x, int y)
{
    return array[x + y * x_size];
}

void ArrayObject::set_value(double value, int x, int y)
{
/*    std::cout << "Set value: " << value << " " << x << " " << y << " "
        << x_size << " " << y_size << " " << z_size << std::endl;*/
    get_value(x, y) = value;
}

ArrayObject::~ArrayObject()
{
    delete array;
}

void ArrayObject::clear()
{
    delete array;
    array = new double[x_size * y_size * z_size]();
}

// LayerObject

int LayerObject::sort_index;
bool LayerObject::sort_reverse;
double LayerObject::def;

LayerObject::LayerObject(const std::string & name, int x, int y, int type_id) 
: FrameObject(name, x, y, type_id), current_layer(0)
{

}

void LayerObject::set_layer(int value)
{
    current_layer = value;
}

void LayerObject::hide_layer(int index)
{
    frame->layers[index]->visible = false;
}

void LayerObject::show_layer(int index)
{
    frame->layers[index]->visible = true;
}

double LayerObject::get_alterable(FrameObject * instance)
{
    if (instance->values == NULL)
        return def;
    return instance->values->get(sort_index);
} 

bool LayerObject::sort_func(FrameObject * a, FrameObject * b)
{
    double value1 = get_alterable(a);
    double value2 = get_alterable(b);
    if (sort_reverse)
        return value1 < value2;
    else
        return value1 > value2;
}

void LayerObject::sort_alt_decreasing(int index, double def)
{
    sort_index = index;
    sort_reverse = true;
    this->def = def;
    ObjectList & instances = frame->layers[current_layer]->instances;
    std::sort(instances.begin(), instances.end(), sort_func);
}

// ActivePicture

ActivePicture::ActivePicture(const std::string & name, int x, int y, 
                             int type_id) 
: FrameObject(name, x, y, type_id), image(NULL), horizontal_flip(false),
  scale_x(1.0), scale_y(1.0), angle(0.0), has_transparent_color(false)
{
    create_alterables();
    collision = new SpriteCollision(this, NULL);
}

ActivePicture::~ActivePicture()
{
    remove_image();
    delete collision;
}

void ActivePicture::remove_image()
{
    if (image == NULL)
        return;
    delete image;
    image = NULL;
}

void ActivePicture::load(const std::string & fn)
{
    remove_image();
    filename = convert_path(fn);
    ImageCache::const_iterator it = image_cache.find(filename);
    if (it == image_cache.end()) {
        Color * transparent = NULL;
        if (has_transparent_color)
            transparent = &transparent_color;
        cached_image = new Image(filename, 0, 0, 0, 0, transparent);
        if (cached_image->image == NULL) {
            delete cached_image;
            cached_image = NULL;
        }
        image_cache[filename] = cached_image;
    } else {
        cached_image = it->second;
    }

    if (cached_image != NULL) {
        image = new Image(*cached_image);
        collision->set_image(image);
        image->hotspot_x = image->hotspot_y = 0;
    }
}

void ActivePicture::set_transparent_color(const Color & color)
{
    transparent_color = color;
    has_transparent_color = true;
}

void ActivePicture::set_hotspot(int x, int y)
{
    if (image == NULL)
        return;
    this->x += x - image->hotspot_x;
    this->y += y - image->hotspot_y;
    image->hotspot_x = x;
    image->hotspot_y = y;
}

void ActivePicture::set_hotspot_mul(float x, float y)
{
    if (image == NULL)
        return;
    set_hotspot(image->width * x, image->height * y);
}

void ActivePicture::flip_horizontal()
{
    horizontal_flip = !horizontal_flip;
}

void ActivePicture::set_scale(double value)
{
    collision->set_scale(value);
    scale_x = scale_y = value;
}

void ActivePicture::set_zoom(double value)
{
    set_scale(value / 100.0);
}

void ActivePicture::set_angle(double value)
{
    collision->set_angle(value);
    angle = value;
}

double ActivePicture::get_zoom_x()
{
    return scale_x * 100.0;
}

int ActivePicture::get_width()
{
    if (image == NULL)
        return 0;
    return image->width;
}

int ActivePicture::get_height()
{
    if (image == NULL)
        return 0;
    return image->height;
}

void ActivePicture::draw()
{
    if (image == NULL)
        return;
    blend_color.apply();
    draw_image(image, x, y, angle, scale_x, scale_y, horizontal_flip);
}

CollisionBase * ActivePicture::get_collision()
{
    return collision;
}

void ActivePicture::paste(int dest_x, int dest_y, int src_x, int src_y, 
                          int src_width, int src_height, int collision_type)
{
    if (image == NULL) {
        std::cout << "Invalid image paste: " << filename << std::endl;
        return;
    }
    frame->layers[layer_index]->paste(cached_image, dest_x, dest_y, 
        src_x, src_y, src_width, src_height, collision_type);
}

ImageCache ActivePicture::image_cache;

// ListObject

ListObject::ListObject(const std::string & name, int x, int y, 
                       int type_id) 
: FrameObject(name, x, y, type_id)
{

}

void ListObject::load_file(const std::string & name)
{
    FSFile fp(name.c_str(), "r");
    if (!fp.is_open())
        return;
    std::string line;
    while (!fp.at_end()) {
        fp.read_line(line);
        add_line(line);
    }
}

void ListObject::add_line(const std::string & value)
{
    lines.push_back(value);
}

const std::string & ListObject::get_line(int i)
{
    return lines[i];
}

int ListObject::get_count()
{
    return lines.size();
}

// MathHelper

MathHelper math_helper;