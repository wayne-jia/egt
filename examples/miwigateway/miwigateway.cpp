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
#include <condition_variable>
#include <sys/stat.h>
#include <functional>
#include <egt/ui>
#include <mosquittopp.h>
extern "C" {
#include "cjson/cJSON.h"
}

#define SERIALPORT

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
	SerialPort *tty_ptr = nullptr;
	mutex mtx;

	Lights() {}
	~Lights() {
		if (lights != nullptr)
			delete [] lights;

		if (mosq_connected)
			disconnect();
	}

	int config_load();
	void message(const char *report, Light *light);
	void update(Light *light, int status, string& temp, string& hum, string& uv);
	void routine();
	void add_text();
	void clear_text();
	void remove_text();

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
	char buf[255];
};

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
				message((char *)msg->payload, &lights[i]);
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

int Lights::config_load()
{
	int result = -1;
	char *buffer;
	struct stat state;
	cJSON *json;
	cJSON *j_lights, *j_light, *j_name, *j_mac, *j_topic_pub, *j_topic_sub;
	ifstream infile;

	if (stat(config_file.c_str(), &state)) {
		cout << "ERROR: stat() " << config_file << endl;
		perror("stat()");
		return -1;
	}
	config_timestamp = state.st_mtime;

	buffer = new char[state.st_size+1];
	if (buffer == nullptr) {
		cout << "ERROR: new " << endl;
		return -1;
	}

	infile.open(config_file);
	if (!infile) {
		cout << "ERROR: open " << config_file << endl;
		goto EXIT;
	}

	infile.read(buffer, state.st_size);
	if (infile.gcount() != state.st_size) {
		cout << "ERROR: read " << infile.gcount() << ", total size " << state.st_size << endl;
		goto EXIT2;
	}

	json = cJSON_Parse(buffer);
	if (!json) {
		cout << "ERROR: cJSON_Parse" << endl;
		goto EXIT2;
	}

	j_lights = cJSON_GetObjectItem(json, json_lights.c_str());
	if (!j_lights) {
		cout << "ERROR: cJSON_GetObjectItem lights" << endl;
		goto EXIT3;
	}

	if (j_lights->type != cJSON_Array) {
		cout << "ERROR: j_lights->type= " << j_lights->type << endl;
		goto EXIT3;
	}

	light_num = cJSON_GetArraySize(j_lights);
	if (light_num < 0) {
		cout << "Error light number: " << light_num << endl;
		goto EXIT3;
	} else if (light_num == 0) {
		result = 0;
		goto EXIT3;
	}

	if (lights != nullptr)
		delete [] lights;
	lights = new Light[light_num];
	if (lights == nullptr) {
		cout << "ERROR: new Light[]" << endl;
		light_num = 0;
		goto EXIT3;
	}

	for (auto i=0; i<light_num; i++) {
		j_light = cJSON_GetArrayItem(j_lights, i);
		if (!j_light) {
			cout << "ERROR: cJSON_GetArrayItem " << i << endl;
			goto EXIT4;
		}

		j_name      = cJSON_GetObjectItem(j_light, json_name.c_str());
		j_mac       = cJSON_GetObjectItem(j_light, json_mac.c_str());
		j_topic_pub = cJSON_GetObjectItem(j_light, json_topic_pub.c_str());
		j_topic_sub = cJSON_GetObjectItem(j_light, json_topic_sub.c_str());
		if ((!j_name || !j_mac || !j_topic_pub || !j_topic_sub) || \
				(j_name->type != cJSON_String || \
					j_mac->type != cJSON_String || \
					j_topic_pub->type != cJSON_String || \
					j_topic_sub->type != cJSON_String)) {
			cout << "ERROR: get name and topic" << endl;
			goto EXIT4;
		}

		lights[i].name      = j_name->valuestring;
		lights[i].mac       = j_mac->valuestring;
		lights[i].topic_pub = j_topic_pub->valuestring;
		lights[i].topic_sub = j_topic_sub->valuestring;
		lights[i].tp        = steady_clock::now();
	}

	result = 0;
	goto EXIT3;
EXIT4:
	delete [] lights;
	lights = nullptr;
	light_num = 0;
EXIT3:
	cJSON_Delete(json);
EXIT2:
	infile.close();
EXIT:
	delete buffer;
	return result;
}

void Lights::add_text()
{
	for (auto i=0; i<light_num; i++) {
		lights[i].mtx.lock();
		lights[i].texts[0] = make_shared<egt::TextBox>(to_string(i+1).c_str(), Rect(Point(0, i*40), Size(30, 30)), AlignFlag::center);
		lights[i].texts[1] = make_shared<egt::TextBox>(lights[i].name.c_str(), Rect(Point(52, i*40), Size(120, 30)), AlignFlag::center);
		lights[i].texts[2] = make_shared<egt::TextBox>("", Rect(Point(169, i*40), Size(100, 30)), AlignFlag::center);
		lights[i].texts[3] = make_shared<egt::TextBox>("", Rect(Point(304, i*40), Size(60, 30)), AlignFlag::center);
		lights[i].texts[4] = make_shared<egt::TextBox>("", Rect(Point(422, i*40), Size(60, 30)), AlignFlag::center);
		lights[i].texts[5] = make_shared<egt::TextBox>("", Rect(Point(530, i*40), Size(60, 30)), AlignFlag::center);
		lights[i].button   = make_shared<egt::ToggleBox>(Rect(Point(184, i*40), Size(70, 30)));

		lights[i].button->toggle_text("On", "Off");
		auto t      = this;
		auto button = lights[i].button.get();
		auto mac    = lights[i].mac;
		auto handle = [t, button, mac](Event & event)
		{
			if (event.id() == EventId::pointer_click) {
				if (button->checked())
					t->mosq_light_on(mac);
				else
					t->mosq_light_off(mac);
			}
		};
		lights[i].button->on_event(handle);
		view_ptr->add(lights[i].button); // Hidden with default

		for (auto j=0; j<6; j++) {
			lights[i].texts[j]->font(Font(20));
			lights[i].texts[j]->readonly(true);
			lights[i].texts[j]->border(false);
			view_ptr->add(lights[i].texts[j]);
		}
		lights[i].mtx.unlock();
	}

	return;
}

void Lights::clear_text()
{
	for (auto i=0; i<light_num; i++) {
		lights[i].texts[2]->zorder_top();
		lights[i].texts[0]->clear();
		lights[i].texts[1]->clear();
		lights[i].texts[2]->clear();
		lights[i].texts[3]->clear();
		lights[i].texts[4]->clear();
		lights[i].texts[5]->clear();
	}

	return;
}

void Lights::remove_text()
{
	view_ptr->remove_all();

	for (auto i=0; i<light_num; i++) {
		for (auto j=0; j<6; j++)
			lights[i].texts[j].reset();
		lights[i].button.reset();
	}

	return;
}

void Lights::routine()
{
	struct stat state;
#ifdef SERIALPORT
	bzero(buf, sizeof(buf));
	//tty_ptr->write("helloegt", 9);
	//cout << "tty0 write done" << endl;
	tty_ptr->read(buf, 10);
	cout << "tty0 read: " << buf << endl;
#endif

	if (!stat(config_file.c_str(), &state)) {
		if (config_timestamp != state.st_mtime) {
			mtx.lock();
			clear_text();
			app_ptr->event().step(); // Workaround here
			remove_text();
			app_ptr->event().step(); // Workaround here
			config_load();
			add_text();
			mtx.unlock();
			return;
		}
	} else {
		cout << "ERROR: stat() " << config_file << endl;
		perror("stat()");
	}

	auto now = steady_clock::now();
	for (auto i=0; i<light_num; i++) {
		if ((lights[i].status != Light::LIGHT_OFFLINE) && (now >= lights[i].tp + seconds(4))) {
			lights[i].mtx.lock();
			lights[i].status = Light::LIGHT_OFFLINE;
			//lights[i].texts[2]->zorder_top();
			lights[i].texts[2]->zorder_bottom();
			lights[i].texts[2]->text("Offline");
			lights[i].texts[3]->clear();
			lights[i].texts[4]->clear();
			lights[i].texts[5]->clear();
			lights[i].mtx.unlock();
		}
	}

	return;
}

void Lights::message(const char *report, Light *light)
{
	int status;
	string temp, hum, uv;
	string::size_type resize;
	cJSON *json_report, *json_params, *json_light, *json_temp, *json_hum, *json_uv;

	status = Light::LIGHT_ERROR;

	json_report = cJSON_Parse(report);
	if (json_report) {
		json_params = cJSON_GetObjectItem(json_report, "params");
		if (json_params) {
			json_light = cJSON_GetObjectItem(json_params, "light_switch");
			if (json_light) {
				if (json_light->valueint == 1)
					status = Light::LIGHT_ON;
				else if (json_light->valueint == 0)
					status = Light::LIGHT_OFF;
			}
			json_temp = cJSON_GetObjectItem(json_params, "temp");
			if (json_temp) {
				temp = to_string(json_temp->valuedouble);
				resize = temp.find(".");
				if (resize != string::npos)
					temp.resize(resize+3);
			}
			json_hum = cJSON_GetObjectItem(json_params, "hum");
			if (json_hum) {
				hum = to_string(json_hum->valueint);
			}
			json_uv = cJSON_GetObjectItem(json_params, "uv");
			if (json_uv) {
				uv = to_string(json_uv->valueint);
			}
		}
		cJSON_Delete(json_report);
	}

	light->tp = steady_clock::now();
	update(light, status, temp, hum, uv);

	return;
}

void Lights::update(Light *light, int status, string& temp, string& hum, string& uv)
{
	int text_cleared = 0;

	light->mtx.lock();

	if ((light->status == Light::LIGHT_OFFLINE) || (light->status == Light::LIGHT_ERROR))
		text_cleared = 1;

	if (light->status != status) {
		light->status = status;
		if (light->status == Light::LIGHT_ERROR) {

			#if 0
			asio::post(egt::Application::instance().event().io(), std::bind(&egt::TextBox::text, light->texts[2], "Error"));
			asio::post(egt::Application::instance().event().io(), std::bind(&egt::TextBox::zorder_top, light->texts[2]));
			asio::post(egt::Application::instance().event().io(), std::bind(&egt::TextBox::clear, light->texts[3]));
			asio::post(egt::Application::instance().event().io(), std::bind(&egt::TextBox::clear, light->texts[4]));
			asio::post(egt::Application::instance().event().io(), std::bind(&egt::TextBox::clear, light->texts[5]));
			#endif

			light->mtx.unlock();
			return;
		} else {
			#if 0
			if (light->status == Light::LIGHT_ON)
				asio::post(egt::Application::instance().event().io(), std::bind(&egt::ToggleBox::checked, light->button, 1));
			else if (light->status == Light::LIGHT_OFF)
				asio::post(egt::Application::instance().event().io(), std::bind(&egt::ToggleBox::checked, light->button, 0));
			asio::post(egt::Application::instance().event().io(), std::bind(&egt::TextBox::clear, light->texts[2]));
			asio::post(egt::Application::instance().event().io(), std::bind(&egt::ToggleBox::zorder_top, light->button));
			#endif
		}
	}

#if 0
	if ((light->temp != temp) || (text_cleared)) {
		light->temp = temp;
		asio::post(egt::Application::instance().event().io(), std::bind(&egt::TextBox::text, light->texts[3], light->temp.c_str()));
	}
	if ((light->hum != hum) || (text_cleared)) {
		light->hum = hum;
		asio::post(egt::Application::instance().event().io(), std::bind(&egt::TextBox::text, light->texts[4], light->hum.c_str()));
	}
	if ((light->uv != uv) || (text_cleared)) {
		light->uv = uv;
		asio::post(egt::Application::instance().event().io(), std::bind(&egt::TextBox::text, light->texts[5], light->uv.c_str()));
	}
#endif

	light->mtx.unlock();

	return;
}

#include <sys/mman.h>
#include <execinfo.h>
#include <signal.h>

void sig_func(int sig)
{
	int j, nptrs;
#define SIZE 100
	void *buffer[SIZE];
	char **strings;

	printf("Error: signal %d:\n", sig);

	nptrs = backtrace(buffer, SIZE);
	printf("backtrace() returned %d addresses\n", nptrs);

	 /* The call backtrace_symbols_fd(buffer, nptrs, STDOUT_FILENO)
		would produce similar output to the following: */

	strings = backtrace_symbols(buffer, nptrs);
	if (strings == NULL) {
		perror("backtrace_symbols");
		exit(EXIT_FAILURE);
	}

	for (j = 0; j < nptrs; j++)
		printf("%s\n", strings[j]);

	free(strings);
	exit(1);
}

int main(int argc, const char** argv)
{
	signal(SIGSEGV, sig_func);

	auto gateway = make_shared<Lights>();
	auto app = make_shared<egt::Application>(0, nullptr);
	auto window = make_shared<egt::TopWindow>();

#ifdef SERIALPORT
	cout << "construct serialport" << endl;
	auto tty0 = make_shared<SerialPort>("/dev/tty0");
	cout << "is tty0 open: " << tty0->isOpen() << endl;
	if ((gateway->tty_ptr = tty0.get()) == nullptr) {
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

	auto title = make_shared<egt::Label>("ID           Name               Light        Temperature    RSSI    Customized",
										Rect(Point(0, 100), Size(window->width(), 30)), AlignFlag::left);
	title->font(Font(20, Font::Weight::bold));
	window->add(title);

	auto view = make_shared<egt::ScrolledView>(Rect(0, 130, window->width(), window->height()-100));
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
	if (gateway->config_load()) {
		cout << "ERROR config_load()" << endl;
		exit(EXIT_FAILURE);
	}
	gateway->add_text();

	window->show();

	if (gateway->mosq_connect()) {
		cout << "ERROR mosq_connect()" << endl;
		exit(EXIT_FAILURE);
	}

	auto timer = make_shared<egt::PeriodicTimer>(std::chrono::seconds(1));
	timer->on_timeout([gateway]()
	{
		gateway->routine();
	});
	timer->start();

	//app->event().add_idle_callback([gateway]()
	//{
	//	gateway->routine();
	//	cout << "haha idle" << endl;
	//});

	return app->run();
}
