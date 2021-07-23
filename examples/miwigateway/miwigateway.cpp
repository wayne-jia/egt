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
#include <condition_variable>
#include <sys/stat.h>
#include <functional>
#include <egt/ui>
#include <mosquittopp.h>
extern "C" {
#include "cjson/cJSON.h"
}

//#define SERIALPORT

#define SCROLLVIEWSIZE 10

#ifdef SERIALPORT
#include "serialport.h"
#endif

using namespace std;
using namespace chrono;
using namespace egt;

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

	shared_ptr<egt::v1::TextBox>   texts[6]; // 0:ID 1:Name 2:Light 3:Temp 4:Hum 5:Uv
	shared_ptr<egt::v1::ToggleBox> button;
	steady_clock::time_point tp;
	mutex mtx;
};

class Lights : public mosqpp::mosquittopp
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
	egt::Application *app_ptr = nullptr;
	egt::ScrolledView *view_ptr = nullptr;
#ifdef SERIALPORT
	SerialPort *tty_ptr = nullptr;
#endif
	mutex mtx;

	Lights() {}
	~Lights() {
		if (lights != nullptr)
			delete [] lights;

		if (mosq_connected)
			disconnect();
	}

	void report_load();
	void routine();
	void init_scrol_view();
	void clear_text();

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
	char m_buf[128];
	std::string m_str;
	std::string last_line(const std::string& str)
	{
		std::stringstream ss(str);
		std::string line;
		while (std::getline(ss, line, '\n')) {}
		return line;
	}
};

void Lights::clear_text()
{
	for (auto i = 0; i < light_num; i++) {
		lights[i].texts[0]->hide();
		lights[i].texts[1]->hide();
		lights[i].texts[2]->hide();
		lights[i].texts[3]->hide();
		lights[i].texts[4]->hide();
		lights[i].texts[5]->hide();
		lights[i].button->hide();
	}

	return;
}

void Lights::on_connect(int rc)
{
	if (!rc) {
		cout << "sub connected " << mqtt_topic_sub << endl;
		mosq_connected = 1;
		auto topic = mqtt_topic_root + "/+" + mqtt_topic_sub;
		subscribe(NULL, topic.c_str(), 0);
	}
}

void Lights::on_message(const struct mosquitto_message *msg)
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

void Lights::on_disconnect(int rc)
{
	cout << "sub disconnect " << rc << endl;
	mosq_connected = 0;
	if (rc) {
		reconnect();
		cout << "sub reconnect " << endl;
	}
}



void Lights::report_load()
{
	std::string temp;
	std::string rssi;
	std::string custm;
	clear_text();
#ifdef SERIALPORT
	std::string line = last_line(m_buf);
#else
	std::string line = m_str;
#endif

	if (line.length())
	{
		std::size_t substrpos = line.find("cmd");
		if (substrpos != std::string::npos)
		{
			//cout << "substrpos: " << substrpos << endl;
			std::string substr = line.substr(substrpos);
			cout << "substr: " << substr << endl;
			light_num = std::stoi(substr.substr(6, 1));
			cout << "lightnum: " << light_num << endl;
			temp = substr.substr(3, 2);
			cout << "temp: " << temp << endl;
			rssi = substr.substr(7, 2);
			cout << "rssi: " << rssi << endl;
			custm = substr.substr(9, 1);
			cout << "custm: " << custm << endl;
			for (auto i = 0; i < light_num; i++) {
				lights[i].texts[0]->text(std::to_string(i));
				lights[i].texts[0]->show();
				lights[i].texts[1]->text("Device" + std::to_string(i));
				lights[i].texts[1]->show();
				lights[i].texts[3]->text(temp);
				lights[i].texts[3]->show();
				lights[i].texts[4]->text(rssi);
				lights[i].texts[4]->show();
				lights[i].texts[5]->text(custm);
				lights[i].texts[5]->show();
				lights[i].button->show();
			}
		}
	}

}


void Lights::init_scrol_view()
{
	if (lights != nullptr)
		delete [] lights;
	lights = new Light[SCROLLVIEWSIZE];
	if (lights == nullptr) {
		cout << "ERROR: new Light[]" << endl;
		return;
	}

	for (auto i = 0; i < SCROLLVIEWSIZE; i++) {
		lights[i].texts[0] = make_shared<egt::TextBox>(std::to_string(i), Rect(Point(0, i*40), Size(30, 30)), AlignFlag::center);
		lights[i].texts[1] = make_shared<egt::TextBox>("Device" + std::to_string(i), Rect(Point(52, i*40), Size(120, 30)), AlignFlag::center);
		lights[i].texts[2] = make_shared<egt::TextBox>("Offline", Rect(Point(189, i*40), Size(100, 30)), AlignFlag::center);
		lights[i].texts[3] = make_shared<egt::TextBox>("78", Rect(Point(333, i*40), Size(60, 30)), AlignFlag::center);
		lights[i].texts[4] = make_shared<egt::TextBox>("90", Rect(Point(448, i*40), Size(60, 30)), AlignFlag::center);
		lights[i].texts[5] = make_shared<egt::TextBox>("a1", Rect(Point(552, i*40), Size(60, 30)), AlignFlag::center);
		lights[i].button   = make_shared<egt::ToggleBox>(Rect(Point(184, i*40), Size(70, 30)));
		lights[i].button->toggle_text("On", "Off");
		lights[i].button->zorder_top();
		lights[i].button->hide();
		view_ptr->add(lights[i].button); // Hidden with default

		for (auto j=0; j<6; j++) {
			lights[i].texts[j]->font(Font(25));
			lights[i].texts[j]->readonly(true);
			lights[i].texts[j]->border(false);
			lights[i].texts[j]->hide();
			view_ptr->add(lights[i].texts[j]);
		}
	}
}


void Lights::routine()
{
	bzero(m_buf, sizeof(m_buf));
	m_str.clear();
#ifdef SERIALPORT
	m_str = "get ver\r";
	cout << "write: " << m_str << endl;
	tty_ptr->write(m_str.data(), m_str.length());
	//tty_ptr->flush();
	tty_ptr->read(m_buf, 100);
	cout << "tty read: " << m_buf << endl;
#else
	std::default_random_engine generator(time(0));
	std::uniform_int_distribution<unsigned long> distribution(4523801,9999999);
	auto dice = std::bind(distribution, generator);
	m_str = "cmd" + std::to_string(dice());
#endif
	report_load();
}


int main(int argc, const char** argv)
{
	detail::ignoreparam(argc);
	detail::ignoreparam(argv);

	auto gateway = make_shared<Lights>();
	auto app = make_shared<egt::Application>(0, nullptr);
	auto window = make_shared<egt::TopWindow>();

#ifdef SERIALPORT
	//cout << "construct serialport" << endl;
	//auto tty = make_shared<SerialPort>("/dev/ttyS3", SerialPort::defaultOptions);
	auto tty = make_shared<SerialPort>("/dev/ttyACM0");
	cout << "is ttyACM0 open: " << tty->isOpen() << endl;
	if ((gateway->tty_ptr = tty.get()) == nullptr) {
		cout << "ERROR get tty_ptr" << endl;
		exit(EXIT_FAILURE);
	}
#endif

	auto img = make_shared<egt::ImageLabel>(Image("file:logo.png"));
	img->align(AlignFlag::left);
	window->add(img);

	auto header = make_shared<egt::Label>("Mi-Wi IoT Control Center",
										Rect(Point(0, 25), Size(window->width(), 100)),
										AlignFlag::left);
	header->font(Font(30, Font::Weight::bold));
	window->add(header);

	auto title = make_shared<egt::Label>("ID             Name               Light          Temperature      RSSI      Customized",
										Rect(Point(0, 100), Size(window->width(), 30)), AlignFlag::left);
	title->font(Font(20, Font::Weight::bold));
	window->add(title);

	auto view = make_shared<egt::ScrolledView>(Rect(0, 140, window->width(), window->height()));
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

	PeriodicTimer timer(std::chrono::seconds(3));
	timer.on_timeout([gateway]()
	{
		gateway->routine();
	});
	timer.start();

	return app->run();
}
