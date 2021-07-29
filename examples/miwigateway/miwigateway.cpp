/*
 * gateway.cpp
 *
 *  Created on: Oct 21, 2019
 *      Author: Xing.Chen
 *  Updated on: Jul 15, 2021
 *      Author: Wayne.Jia
 */

#include <iostream>
#include <fstream>
#include <unistd.h>
#include <string>
#include <thread>
#include <mutex>
#include <random>
#include <regex>
#include <condition_variable>
#include <sys/stat.h>
#include <functional>
#include <cxxopts.hpp>
#include <egt/ui>
#include <mosquittopp.h>
extern "C" {
#include "cjson/cJSON.h"
}

//#define DBGMODE
#define SERIALPORT

#define SCROLLVIEWSIZE 10
#define SCROLLVIEWY    140

#ifdef SERIALPORT
#include "serialport.h"
#endif

using namespace std;
using namespace chrono;
using namespace egt;

typedef struct {
	std::string index;
	int valid;
	bool led;
	bool gpio1;
	bool gpio2;
	std::string addr;
	std::string light;
	std::string temp;
	std::string rssi;
	std::string casttype;
}miwi_dev_st;

class Light
{
public:
	enum {
		LIGHT_NONE = 0,
		LIGHT_OFF,
		LIGHT_ON,
		LIGHT_OFFLINE,
		LIGHT_ERROR
	};

	int status = LIGHT_NONE;
	string name;
	string mac;
	string topic_pub;
	string topic_sub;
	string temp;
	string hum;
	string uv;
	miwi_dev_st dev_attr;

	shared_ptr<egt::v1::TextBox>   texts[5]; // 0:ID 1:Name 2:Light 3:Temp 4:Hum 5:Uv
	shared_ptr<egt::v1::ToggleBox> button;
	shared_ptr<egt::v1::ToggleBox> button1;
	shared_ptr<egt::v1::ToggleBox> button2;
	steady_clock::time_point tp;
	mutex mtx;
};

class MiWiGtw : public mosqpp::mosquittopp
{
public:
	const string config_file     = "/var/www/cgi-bin/lights.json";
	const string json_lights     = "lights";
	const string json_name       = "name";
	const string json_mac        = "mac";
	const string json_topic_pub  = "topic_publish";
	const string json_topic_sub  = "topic_subscribe";
	const string mqtt_host       = "localhost";
	const    int mqtt_port       = 1883;
	const    int mqtt_alive      = 60;
	/* MQTT topic = root + /mac + sub/pub */
	const string mqtt_topic_root = "/end_node";
	const string mqtt_topic_sub  = "/sensor_board/data_report";
	const string mqtt_topic_pub  = "/sensor_board/data_control";
	const string mqtt_cmd_off    = "{\"id\":\"1\",\"version\":\"1.0\",\"params\":{\"light_switch\":0,\"light_intensity\":88,\"led_r\":0,\"led_g\":0,\"led_b\":0}}";
	const string mqtt_cmd_on     = "{\"id\":\"1\",\"version\":\"1.0\",\"params\":{\"light_switch\":1,\"light_intensity\":88,\"led_r\":0,\"led_g\":0,\"led_b\":0}}";

	int light_num = 0;
	Light *lights = nullptr;
	time_t config_timestamp = 0;
	int mosq_connected = 0;
	Application *app_ptr = nullptr;
	ScrolledView *view_ptr = nullptr;
	PeriodicTimer *timer_ptr = nullptr;
#ifdef SERIALPORT
	SerialPort *tty_ptr = nullptr;
#endif
	mutex mtx;

	MiWiGtw() {}
	~MiWiGtw() {
		if (lights != nullptr)
			delete [] lights;

		if (mosq_connected)
			disconnect();
	}

	void report_load();
	void routine();
	bool initconn();
	void init_scrol_view();
	void clear_text();
	void show_text();
	void add_rm_end_node_device(std::string msg);
	void update_end_node_device(std::string msg);
	bool is_point_in_list(DisplayPoint& point, int* index);
	bool is_point_in_rect(DisplayPoint& point, Rect rect);

	int mosq_connect(void) {
		if (connect(mqtt_host.c_str(), mqtt_port, mqtt_alive))
			return -1;
		return loop_start();
	}

	int mosq_publish(const string& topic, const string& msg) {
		return publish(nullptr, topic.c_str(), msg.length(), msg.c_str(), 1, false);
	}

	int mosq_light_on(const string& mac) {
		auto topic = mqtt_topic_root + "/" + mac + mqtt_topic_pub;
		return mosq_publish(topic, mqtt_cmd_on);
	}

	int mosq_light_off(const string& mac) {
		auto topic = mqtt_topic_root + "/" + mac + mqtt_topic_pub;
		return mosq_publish(topic, mqtt_cmd_off);
	}

	void on_connect(int rc);
	void on_message(const struct mosquitto_message *msg);
	void on_disconnect(int rc);

private:
	int m_idx;
	char m_buf[128];
	std::string m_str;
	char m_buf_asyn[128];
	std::string m_str_asyn;
	std::string last_line(const std::string& str)
	{
		std::stringstream ss(str);
		std::string line;
		while (std::getline(ss, line, '\n')) {}
		return line;
	}

	bool check_if_hex(const std::string& str)
	{
		std::string pattern = "[0-9a-fA-F]+";
    regex re(pattern);
		if (regex_match(str, re))
			return true;
		else
			return false;
	}

	std::string convert_2_db(int rssi)
	{
		int dec = 0xff & rssi;
		dec = (~dec & 0xff) + 1;
		return "-" + std::to_string(dec) + "dBm";
	}
};

inline constexpr std::uint32_t hash_str_to_uint32(const char* data)
{
	std::uint32_t h(0);
	for (int i = 0; data && ('\0' != data[i]); i++)
		h = (h << 6) ^ (h >> 26) ^ data[i];
	return h;
}

void MiWiGtw::on_connect(int rc)
{
	if (!rc) {
		cout << "sub connected " << mqtt_topic_sub << endl;
		mosq_connected = 1;
		auto topic = mqtt_topic_root + "/+" + mqtt_topic_sub;
		subscribe(NULL, topic.c_str(), 0);
	}
}

void MiWiGtw::on_message(const struct mosquitto_message *msg)
{
	if (!mtx.try_lock())
		return;

	if (msg->payloadlen) {
		for (auto i=0; i<light_num; i++)
			if (strstr(msg->topic, lights[i].mac.c_str()))
				cout << "on msg: " << msg->payload << endl;
	}

	mtx.unlock();
}

void MiWiGtw::on_disconnect(int rc)
{
	cout << "sub disconnect " << rc << endl;
	mosq_connected = 0;
	if (rc) {
		reconnect();
		cout << "sub reconnect " << endl;
	}
}

void MiWiGtw::clear_text()
{
	for (auto i = 0; i < light_num; i++) {
		lights[i].texts[0]->hide();
		lights[i].texts[1]->hide();
		lights[i].texts[2]->hide();
		lights[i].texts[3]->hide();
		lights[i].texts[4]->hide();
		lights[i].button->hide();
		lights[i].button1->hide();
		lights[i].button2->hide();
	}

	return;
}

void MiWiGtw::show_text()
{
	for (auto i = 0; i < light_num; i++) {
		lights[i].texts[0]->text(lights[i].dev_attr.index);
		lights[i].texts[0]->show();
		lights[i].texts[1]->text("Device" + lights[i].dev_attr.index);
		lights[i].texts[1]->show();
		if (!lights[i].dev_attr.valid) {
			lights[i].texts[2]->show();
			lights[i].button->hide();
			lights[i].button1->hide();
			lights[i].button2->hide();
			lights[i].texts[3]->hide();
			lights[i].texts[4]->hide();
		} else {
			lights[i].texts[2]->hide();
			lights[i].button->show();
			lights[i].button1->show();
			lights[i].button2->show();
			lights[i].texts[3]->text(lights[i].dev_attr.temp);
			lights[i].texts[3]->show();
			lights[i].texts[4]->text(lights[i].dev_attr.rssi);
			lights[i].texts[4]->show();
		}
	}
	return;
}

void MiWiGtw::add_rm_end_node_device(std::string msg)
{
	if (!check_if_hex(msg.substr(5, 1)) || !check_if_hex(msg.substr(7, 1)))
		return;
	int i = std::stoi(msg.substr(5, 1), nullptr, 16);
	if (i+1 > light_num)
		light_num = i + 1;

	if (std::stoi(msg.substr(7, 1), nullptr, 16))
		lights[i].dev_attr.valid = 1;
	else
		lights[i].dev_attr.valid = 0;
	lights[i].dev_attr.led = true;
	lights[i].dev_attr.gpio1 = false;
	lights[i].dev_attr.gpio2 = false;
	lights[i].dev_attr.index = msg.substr(5, 1);
	lights[i].dev_attr.addr = msg.substr(9, 16);
	lights[i].dev_attr.light = "0";
	lights[i].dev_attr.temp = "0";
	lights[i].dev_attr.rssi = "0";
	lights[i].dev_attr.casttype = "0";
}

void MiWiGtw::update_end_node_device(std::string msg)
{
	for (auto i = 0; i < light_num; i++) {
		if (lights[i].dev_attr.addr == msg.substr(11, 16)) {
			if (check_if_hex(msg.substr(8, 2)))
				lights[i].dev_attr.rssi = convert_2_db(std::stoi(msg.substr(8, 2), nullptr, 16));
			lights[i].dev_attr.temp = msg.substr(33, 4) + "°C";
			lights[i].dev_attr.casttype = msg.substr(5, 2);
		}
	}
}

void MiWiGtw::report_load()
{
#ifdef SERIALPORT
	std::string msg = last_line(m_buf);
#else
	std::string msg = m_str;
#endif

	if (msg.length()) {
		timer_ptr->stop();
		switch (hash_str_to_uint32(msg.substr(0, 4).data())) {
			case hash_str_to_uint32("conn"):
				clear_text();
				add_rm_end_node_device(msg);
				break;
			case hash_str_to_uint32("recv"):
				clear_text();
				update_end_node_device(msg);
				break;
			case hash_str_to_uint32("test"):
				break;
			default:
				break;
		}
		show_text();
		timer_ptr->start();
	}
}

bool MiWiGtw::is_point_in_rect(DisplayPoint& point, Rect rect)
{
	if (point.x() >= rect.x()
		&& point.x() <= (rect.x() + rect.width())
		&& point.y() >= rect.y()
		&& point.y() <= (rect.y() + rect.height()))
		return true;
	else
		return false;
}

bool MiWiGtw::is_point_in_list(DisplayPoint& point, int* index)
{
	Rect tmp_rect;
	Rect tmp_rect1;
	Rect tmp_rect2;
	for (auto i = 0; i < light_num; i++) {
		tmp_rect = lights[i].button->box();
		tmp_rect.y(tmp_rect.y() + SCROLLVIEWY);
		tmp_rect1 = lights[i].button1->box();
		tmp_rect1.y(tmp_rect1.y() + SCROLLVIEWY);
		tmp_rect2 = lights[i].button2->box();
		tmp_rect2.y(tmp_rect2.y() + SCROLLVIEWY);

		if (is_point_in_rect(point, tmp_rect)
				|| is_point_in_rect(point, tmp_rect1)
				|| is_point_in_rect(point, tmp_rect2)) {
			*index = i;
			return true;
		}	else
			continue;
	}
	*index = 0XFF;
	return false;
}

void MiWiGtw::init_scrol_view()
{
	int i = 0;
	if (lights != nullptr)
		delete [] lights;
	lights = new Light[SCROLLVIEWSIZE];
	if (lights == nullptr) {
		cout << "ERROR: new Light[]" << endl;
		return;
	}

#if 0
	std::vector<std::shared_ptr<int>> idx;
	for (i = 0; i < SCROLLVIEWSIZE; i++) {
		idx.push_back(make_shared<int>(i));
		cout << "idx:" << *idx[i] << endl;
	}
#endif

	for (i = 0; i < SCROLLVIEWSIZE; i++) {
		lights[i].texts[0] = make_shared<TextBox>(std::to_string(i), Rect(Point(0, i*40), Size(30, 30)), AlignFlag::center);
		lights[i].texts[1] = make_shared<TextBox>("Device" + std::to_string(i), Rect(Point(30, i*40), Size(120, 30)), AlignFlag::center);
		lights[i].texts[2] = make_shared<TextBox>("Offline", Rect(Point(170, i*40), Size(114, 28)), AlignFlag::center);
		lights[i].texts[2]->color(Palette::ColorId::bg, Palette::red);
		lights[i].texts[2]->color(Palette::ColorId::text, Palette::yellow);
		lights[i].texts[3] = make_shared<TextBox>("0", Rect(Point(312, i*40), Size(80, 30)), AlignFlag::center);
		lights[i].texts[4] = make_shared<TextBox>("0", Rect(Point(400, i*40), Size(80, 30)), AlignFlag::center);
		lights[i].button   = make_shared<ToggleBox>(Rect(Point(170, i*40), Size(70, 30)));
		lights[i].button->toggle_text("On", "Off");
		lights[i].button->hide();
		lights[i].button->on_click([&](egt::Event& event) {
			if (!is_point_in_list(event.pointer().point, &m_idx)) {
				cerr << "LED click ERROR range" << endl;
				return;
			}
			lights[m_idx].dev_attr.led = lights[m_idx].dev_attr.led ? false : true;
			if (lights[m_idx].dev_attr.led)
				m_str_asyn = "send " + std::to_string(m_idx) + " 0 LED1 1" + "\r";
			else
				m_str_asyn = "send " + std::to_string(m_idx) + " 0 LED1 0" + "\r";

			cout << "s-> " << m_str_asyn << endl;
			tty_ptr->write(m_str_asyn.data(), m_str_asyn.length());
		});
		lights[i].button1   = make_shared<ToggleBox>(Rect(Point(490, i*40), Size(70, 30)));
		lights[i].button1->toggle_text("0", "1");
		lights[i].button1->hide();
		lights[i].button1->on_click([&](egt::Event& event) {
			if (!is_point_in_list(event.pointer().point, &m_idx)) {
				cerr << "m_gpio1 click ERROR range" << endl;
				return;
			}
			lights[m_idx].dev_attr.gpio1 = lights[m_idx].dev_attr.gpio1 ? false : true;
			if (lights[m_idx].dev_attr.gpio1)
				m_str_asyn = "send " + std::to_string(m_idx) + " 0 GPIO1 1" + "\r";
			else
				m_str_asyn = "send " + std::to_string(m_idx) + " 0 GPIO1 0" + "\r";

			cout << "s-> " << m_str_asyn << endl;
			tty_ptr->write(m_str_asyn.data(), m_str_asyn.length());
		});
		lights[i].button2   = make_shared<ToggleBox>(Rect(Point(650, i*40), Size(70, 30)));
		lights[i].button2->toggle_text("0", "1");
		lights[i].button2->hide();
		lights[i].button2->on_click([&](egt::Event& event) {
			if (!is_point_in_list(event.pointer().point, &m_idx)) {
				cerr << "m_gpio2 click ERROR range" << endl;
				return;
			}
			lights[m_idx].dev_attr.gpio2 = lights[m_idx].dev_attr.gpio2 ? false : true;
			if (lights[m_idx].dev_attr.gpio2)
				m_str_asyn = "send " + std::to_string(m_idx) + " 0 GPIO2 1" + "\r";
			else
				m_str_asyn = "send " + std::to_string(m_idx) + " 0 GPIO2 0" + "\r";

			cout << "s-> " << m_str_asyn << endl;
			tty_ptr->write(m_str_asyn.data(), m_str_asyn.length());
		});
		lights[i].dev_attr.valid = 0;
		view_ptr->add(lights[i].button); // Hidden with default
		view_ptr->add(lights[i].button1);
		view_ptr->add(lights[i].button2);

		for (auto j=0; j<5; j++) {
			lights[i].texts[j]->font(Font(20));
			lights[i].texts[j]->readonly(true);
			lights[i].texts[j]->border(false);
			lights[i].texts[j]->hide();
			view_ptr->add(lights[i].texts[j]);
		}
	}
}

bool MiWiGtw::initconn()
{
#if 0
	bzero(m_buf, sizeof(m_buf));
	m_str = "echo\r";
	cout << "write: " << m_str
			 << " ,len: " << m_str.length() << endl;
	tty_ptr->write(m_str.data(), m_str.length());
	usleep(500000);
	tty_ptr->read(m_buf, 100);
	cout << "tty read: " << m_buf
			 << " ,len: " << last_line(m_buf).length() << endl;
	if ("AOK" != last_line(m_buf).substr(0, 3)) {
		cout << "ERROR echo" << endl;
		return false;
	}
	return true;
#endif

	while ("AOK" != last_line(m_buf).substr(0, 3)) {
		bzero(m_buf, sizeof(m_buf));
		m_str = "cfg channel 8\r";
		cout << "write: " << m_str << endl;
		tty_ptr->write(m_str.data(), m_str.length());
		usleep(500000);
		tty_ptr->read(m_buf, 100);
		cout << "tty read: " << m_buf << " ,len: " << last_line(m_buf).length() << endl;
	}

	bzero(m_buf, sizeof(m_buf));
	m_str = "cfg pan 4321\r";
	cout << "write: " << m_str << endl;
	tty_ptr->write(m_str.data(), m_str.length());
	usleep(500000);
	tty_ptr->read(m_buf, 100);
	cout << "tty read: " << m_buf << " ,len: " << last_line(m_buf).length() << endl;
	if ("AOK" != last_line(m_buf).substr(0, 3)) {
		cout << "ERROR cfg pan 4321" << endl;
		return false;
	}

	bzero(m_buf, sizeof(m_buf));
	m_str = "cfg reconn 2\r";
	cout << "write: " << m_str << endl;
	tty_ptr->write(m_str.data(), m_str.length());
	usleep(500000);
	tty_ptr->read(m_buf, 100);
	cout << "tty read: " << m_buf << " ,len: " << last_line(m_buf).length() << endl;
	if ("AOK" != last_line(m_buf).substr(0, 3)) {
		cout << "ERROR cfg reconn 2" << endl;
		return false;
	}

	bzero(m_buf, sizeof(m_buf));
	m_str = "~cfg\r";
	cout << "write: " << m_str << endl;
	tty_ptr->write(m_str.data(), m_str.length());
	usleep(500000);
	tty_ptr->read(m_buf, 100);
	cout << "tty read: " << m_buf << " ,len: " << last_line(m_buf).length() << endl;
	if ("AOK" != last_line(m_buf).substr(0, 3)) {
		cout << "ERROR ~cfg" << endl;
		return false;
	}

	bzero(m_buf, sizeof(m_buf));
	m_str = "start\r";
	cout << "write: " << m_str << endl;
	tty_ptr->write(m_str.data(), m_str.length());
	usleep(500000);
	tty_ptr->read(m_buf, 100);
	cout << "tty read: " << m_buf << " ,len: " << last_line(m_buf).length() << endl;
	if ("AOK" != last_line(m_buf).substr(0, 3)) {
		cout << "ERROR start" << endl;
		return false;
	}

	return true;
}

void MiWiGtw::routine()
{
#ifdef SERIALPORT
	bzero(m_buf, sizeof(m_buf));
	//m_str = "get ver\r";
	//cout << "write: " << m_str << endl;
	//tty_ptr->write(m_str.data(), m_str.length());
	//tty_ptr->flush();
	tty_ptr->read(m_buf, 100);
	//cout << "tty: " << last_line(m_buf) << " len: " << last_line(m_buf).length() << endl;
#ifdef DBGMODE
	timer_ptr->stop();
	while (1) {
		getline(cin, m_str);
		m_str += "\r";
		cout << "hand write: " << m_str << " ,len: " << last_line(m_str).length() << endl;
		tty_ptr->write(m_str.data(), m_str.length());
		usleep(500000);
		bzero(m_buf, sizeof(m_buf));
		tty_ptr->read(m_buf, 100);
		cout << "tty read: " << m_buf << " ,len: " << last_line(m_buf).length() << endl;
	}
#else
	if (!last_line(m_buf).length())
		return;
#endif
#else
	m_str.clear();
	std::default_random_engine generator(time(0));
	std::uniform_int_distribution<unsigned long> distribution(4523801,9999999);
	auto dice = std::bind(distribution, generator);
	m_str = "cmd" + std::to_string(dice());
#endif

#ifndef DBGMODE
	cout << last_line(m_buf) << endl;
	report_load();
#endif
}


int main(int argc, char** argv)
{
	cxxopts::Options options("miwigateway", "Control Mi-Wi devices via Serial Port");
	options.add_options()
	("h,help", "help")
	("i,input-format", "tty device name",
		cxxopts::value<std::string>()->default_value("ttyS3"))
	("positional", "DEVICE", cxxopts::value<std::vector<std::string>>())
	;
	options.positional_help("DEVICE");

	options.parse_positional({"positional"});
	auto result = options.parse(argc, argv);

	if (result.count("help"))
	{
			cout << options.help() << endl;
			return 0;
	}

	if (result.count("positional") != 1)
	{
			cerr << options.help() << endl;
			return 1;
	}

	auto& positional = result["positional"].as<vector<std::string>>();

	std::string in = positional[0];
	//cout << "user input: " << in << endl;
	auto gateway = make_shared<MiWiGtw>();
	auto app = make_shared<egt::Application>(0, nullptr);
	auto window = make_shared<egt::TopWindow>();

#ifdef SERIALPORT
	in = "/dev/" + in;
	cout << "construct serialport device: " << in << endl;
	auto tty = make_shared<SerialPort>(in);
	//auto tty = make_shared<SerialPort>("/dev/ttyACM0");
	cout << "is ttyS3 open: " << tty->isOpen() << endl;
	if ((gateway->tty_ptr = tty.get()) == nullptr) {
		cout << "ERROR get tty_ptr" << endl;
		exit(EXIT_FAILURE);
	}
#endif

	auto img = make_shared<egt::ImageLabel>(Image("file:logo.png"));
	img->align(AlignFlag::left);
	img->padding(10);
	window->add(img);

	auto header = make_shared<egt::Label>("Mi-Wi IoT Control Center",
										Rect(Point(0, 35), Size(window->width(), 100)),
										AlignFlag::left);
	header->font(Font(30, Font::Weight::bold));
	window->add(header);

	//auto title = make_shared<egt::Label>(" ID        Name                 LED        Temperature  RSSI          GPIO1                   GPIO2",
	//									Rect(Point(0, 100), Size(window->width(), 30)), AlignFlag::left);
	auto title = make_shared<egt::Label>(" ID        Name                 LED        Temperature  RSSI        Clockwise       Counterclockwise",
										Rect(Point(0, 100), Size(window->width(), 30)), AlignFlag::left);
	title->font(Font(20, Font::Weight::bold));
	window->add(title);

	auto view = make_shared<egt::ScrolledView>(Rect(0, SCROLLVIEWY, window->width(), window->height()));
	view->color(Palette::ColorId::bg, Palette::white);
	view->name("view");
	window->add(view);

	if ((gateway->app_ptr = app.get()) == nullptr) {
		cout << "ERROR get app_ptr" << endl;
		exit(EXIT_FAILURE);
	}

	if ((gateway->view_ptr = view.get()) == nullptr) {
		cout << "ERROR get view_ptr" << endl;
		exit(EXIT_FAILURE);
	}

	gateway->init_scrol_view();

	window->show();

	auto timer = make_shared<PeriodicTimer>(std::chrono::milliseconds(300));
	if ((gateway->timer_ptr = timer.get()) == nullptr) {
		cout << "ERROR get timer" << endl;
		exit(EXIT_FAILURE);
	}

	timer->on_timeout([gateway]()
	{
		gateway->routine();
	});

	if (gateway->initconn())
		timer->start();
	else
		exit(EXIT_FAILURE);

	return app->run();
}
