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
#include <sys/time.h>
#include <netdb.h>  //gethostbyname
#include <arpa/inet.h>  //ntohl
#include <functional>
#include <cxxopts.hpp>
#include <egt/ui>
#include <mosquittopp.h>
extern "C" {
#include "cjson/cJSON.h"
}

//#define DBGMODE
#define SERIALPORT
#define MCHPDEMO

#define SCROLLVIEWSIZE 10
#define SCROLLVIEWY    140
#define LEN_CONN_MSG 26
#define LEN_RECV_MSG 38
#define TIMEOUT_DEVICE_OFFLINE 11
#define JSON_LIGHTS    "lights"
#define JSON_WEBUPDA        "webupda"
#define JSON_ID        "id"
#define JSON_NAME		   "name"
#define JSON_VALID		   "valid"
#define JSON_LED		   "led"
#define JSON_TEMPERATURE "temperature"
#define JSON_RSSI "rssi"
#define JSON_CLKWISE "clockwise"
#define JSON_CNTCLKWISE "counterclockwise"
#define JSON_NAME_LEN	12

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
	bool active;
	std::string name;
	std::string ledstr;
	std::string clkwise;
	std::string cntclkwise;
	std::string addr;
	std::string light;
	std::string temp;
	std::string rssi;
	std::string casttype;
}miwi_dev_st;

typedef std::vector<std::pair<int, std::string>> addr_idx_t;
typedef std::vector<std::pair<const std::string, const std::string>> name_table_t;

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
	miwi_dev_st dev_attr;
	struct timeval timestamp;
	shared_ptr<egt::v1::TextBox>   texts[5]; // 0:ID 1:Name 2:Light 3:Temp 4:Hum 5:Uv
	shared_ptr<egt::v1::ToggleBox> button;
	shared_ptr<egt::v1::ToggleBox> button1;
	shared_ptr<egt::v1::ToggleBox> button2;
	steady_clock::time_point tp;
	mutex mtx;
};

class MiWiGtw
{
public:
	const string config_file     = "/tmp/lights.json";
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
	addr_idx_t addr_idx;
	name_table_t name_tbl;
	int index = 0;

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
	}


	cJSON* OpenJson();
	void CloseJson(cJSON *json);
	void SaveJson(cJSON *json);
	void AddJson(miwi_dev_st dev);
	void UpdateJson(miwi_dev_st dev);
	void DeleteJson(cJSON *json_light, int num);
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
	void check_offiline_device();
	void check_json_upda();
	std::string get_local_IP();


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

#if 0
bool MiWiGtw::GetHostInfo(std::string& hostName, std::string& Ip)
{
	char name[256];
	gethostname(name, sizeof(name));
	hostName = name;

	struct hostent* host = gethostbyname(name);
	char ipStr[32];
	const char* ret = inet_ntop(host->h_addrtype, host->h_addr_list[0], ipStr, sizeof(ipStr));
	if (NULL==ret) {
		std::cout << "hostname transform to ip failed";
		return false;
	}
	Ip = ipStr;
	return true;
}
#endif

std::string MiWiGtw::get_local_IP()
{
  int inet_sock;
  struct ifreq ifr;
	char ip[32]={'0'};

	inet_sock = socket(AF_INET, SOCK_DGRAM, 0);
	strcpy(ifr.ifr_name, "eth0");
	ioctl(inet_sock, SIOCGIFADDR, &ifr);
	strcpy(ip, inet_ntoa(((struct sockaddr_in *)&ifr.ifr_addr)->sin_addr));
	return string(ip);
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
		lights[i].texts[1]->text(lights[i].dev_attr.name);
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

void MiWiGtw::check_offiline_device()
{
	bool is_change = false;
	struct timeval curtime;
	gettimeofday(&curtime, NULL);
	for (auto i = 0; i < light_num; i++) {
		if (lights[i].dev_attr.valid && lights[i].dev_attr.active) {
			if (abs(curtime.tv_sec - lights[i].timestamp.tv_sec) > TIMEOUT_DEVICE_OFFLINE) {
				cout << "device[" << i << "] offline" << endl;
				lights[i].dev_attr.valid = 0;
				UpdateJson(lights[i].dev_attr);
				is_change = true;
			}
		}
	}

	if (is_change) {
		clear_text();
		show_text();
	}
}

void MiWiGtw::add_rm_end_node_device(std::string msg)
{
	if (!check_if_hex(msg.substr(5, 1)) || !check_if_hex(msg.substr(7, 1)))
		return;
	bool is_new_node = true;
	int i = 0;
	std::string tmpstr;
	std::string newaddr = msg.substr(9, 16);

	for (auto& idx : addr_idx) {
		if (idx.second == newaddr) {
			i = idx.first;
			is_new_node = false;
			break;
		}
	}

	if (is_new_node) {
		addr_idx.push_back(std::make_pair(index++, newaddr));
		i = index - 1;
		light_num++;

#ifdef MCHPDEMO
		tmpstr = "Device" + std::to_string(i);
		name_tbl.push_back(std::make_pair(tmpstr, newaddr));
#else
		if ("20d137feff19276a" == newaddr)
			name_tbl.push_back(std::make_pair("Rapidlink1", newaddr));
		else if ("1e5f37feff19276a" == newaddr)
			name_tbl.push_back(std::make_pair("Rapidlink2", newaddr));
		else if ("ac3d37feff19276a" == newaddr)
			name_tbl.push_back(std::make_pair("Rapidlink3", newaddr));
		else {
			tmpstr = "Rapidlink" + std::to_string(3+index);
			name_tbl.push_back(std::make_pair(tmpstr, newaddr));
		}
#endif
	}

	lights[i].dev_attr.active = false;
	if (std::stoi(msg.substr(7, 1), nullptr, 16)) {
		lights[i].dev_attr.valid = 1;

	}
	else {
		lights[i].dev_attr.valid = 0;

	}

	lights[i].dev_attr.led = true;
	lights[i].dev_attr.gpio1 = false;
	lights[i].dev_attr.gpio2 = false;
	lights[i].dev_attr.index = std::to_string(i);
	lights[i].dev_attr.addr = newaddr;
	lights[i].dev_attr.light = "0";

	lights[i].dev_attr.casttype = "0";
	for (auto& ind : name_tbl) {
		if (ind.second == lights[i].dev_attr.addr)
			lights[i].dev_attr.name =ind.first;
	}
  if ("0" == lights[i].dev_attr.name) {
		cout << "Device addr: " << lights[i].dev_attr.addr << " not fill in the name table, please add!" << endl;
		lights[i].dev_attr.name = lights[i].dev_attr.addr.substr(0, 7);
	}

	if (is_new_node) {
		lights[i].dev_attr.temp = "0";
		lights[i].dev_attr.rssi = "0";
		lights[i].dev_attr.ledstr = "1";
		lights[i].dev_attr.clkwise = "0";
		lights[i].dev_attr.cntclkwise = "0";
		AddJson(lights[i].dev_attr);
	}

	if (!lights[i].dev_attr.valid)
		UpdateJson(lights[i].dev_attr);

	//for (auto& ind : addr_idx)
	//	cout << "addr table: " << ind.first << "-" << ind.second << endl;
}

void MiWiGtw::update_end_node_device(std::string msg)
{
	struct timeval curtime;
	gettimeofday(&curtime, NULL);
	for (auto i = 0; i < light_num; i++) {
		if (lights[i].dev_attr.addr == msg.substr(11, 16)) {
			if (check_if_hex(msg.substr(8, 2)))
				lights[i].dev_attr.rssi = convert_2_db(std::stoi(msg.substr(8, 2), nullptr, 16));
			lights[i].dev_attr.temp = msg.substr(33, 4) + "C";
			lights[i].dev_attr.casttype = msg.substr(5, 2);
			lights[i].dev_attr.active = true;
			lights[i].dev_attr.valid = 1;
			lights[i].timestamp = curtime;
			UpdateJson(lights[i].dev_attr);
			lights[i].dev_attr.temp = msg.substr(33, 4) + "°C";
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
				if (LEN_CONN_MSG > msg.length())
					break;
				clear_text();
				add_rm_end_node_device(msg);
				break;
			case hash_str_to_uint32("recv"):
				if (LEN_RECV_MSG > msg.length())
					break;
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

#if 1
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
#endif

bool MiWiGtw::is_point_in_list(DisplayPoint& point, int* i_list)
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
			*i_list = i;
			return true;
		}	else
			continue;
	}
	*i_list = 0XFF;
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
		lights[i].button  = make_shared<ToggleBox>(Rect(Point(170, i*40), Size(70, 30)));
		lights[i].button->toggle_text("On", "Off");
		lights[i].button->hide();
		lights[i].button->on_click([&](egt::Event& event) {
			if (!is_point_in_list(event.pointer().point, &m_idx)) {
				cerr << "LED click ERROR range" << endl;
				return;
			}
			lights[m_idx].dev_attr.led = lights[m_idx].dev_attr.led ? false : true;
			if (lights[m_idx].dev_attr.led) {
				m_str_asyn = "send " + std::to_string(m_idx) + " 0 LED1 1" + "\r";
				lights[m_idx].dev_attr.ledstr = "1";
			}	else {
				m_str_asyn = "send " + std::to_string(m_idx) + " 0 LED1 0" + "\r";
				lights[m_idx].dev_attr.ledstr = "0";
			}

			cout << "s-> " << m_str_asyn << endl;
			tty_ptr->write(m_str_asyn.data(), m_str_asyn.length());

			UpdateJson(lights[m_idx].dev_attr);
		});
		lights[i].button1   = make_shared<ToggleBox>(Rect(Point(490, i*40), Size(70, 30)));
		lights[i].button1->toggle_text("1", "0");
		lights[i].button1->hide();
		lights[i].button1->on_click([&](egt::Event& event) {
			if (!is_point_in_list(event.pointer().point, &m_idx)) {
				cerr << "m_gpio1 click ERROR range" << endl;
				return;
			}
			lights[m_idx].dev_attr.gpio1 = lights[m_idx].dev_attr.gpio1 ? false : true;
			if (lights[m_idx].dev_attr.gpio1) {
				m_str_asyn = "send " + std::to_string(m_idx) + " 0 GPIO1 1" + "\r";
				lights[m_idx].dev_attr.clkwise = "1";
			}	else {
				m_str_asyn = "send " + std::to_string(m_idx) + " 0 GPIO1 0" + "\r";
				lights[m_idx].dev_attr.clkwise = "0";
			}

			cout << "s-> " << m_str_asyn << endl;
			tty_ptr->write(m_str_asyn.data(), m_str_asyn.length());

			UpdateJson(lights[m_idx].dev_attr);
		});
		lights[i].button2   = make_shared<ToggleBox>(Rect(Point(650, i*40), Size(70, 30)));
		lights[i].button2->toggle_text("1", "0");
		lights[i].button2->hide();
		lights[i].button2->on_click([&](egt::Event& event) {
			if (!is_point_in_list(event.pointer().point, &m_idx)) {
				cerr << "m_gpio2 click ERROR range" << endl;
				return;
			}
			lights[m_idx].dev_attr.gpio2 = lights[m_idx].dev_attr.gpio2 ? false : true;
			if (lights[m_idx].dev_attr.gpio2) {
				m_str_asyn = "send " + std::to_string(m_idx) + " 0 GPIO2 1" + "\r";
				lights[m_idx].dev_attr.cntclkwise = "1";
			}	else {
				m_str_asyn = "send " + std::to_string(m_idx) + " 0 GPIO2 0" + "\r";
				lights[m_idx].dev_attr.cntclkwise = "0";
			}

			cout << "s-> " << m_str_asyn << endl;
			tty_ptr->write(m_str_asyn.data(), m_str_asyn.length());

			UpdateJson(lights[m_idx].dev_attr);
		});
		lights[i].dev_attr.valid = 0;
		lights[i].dev_attr.active = false;
		lights[i].dev_attr.name = "0";
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

	/*Initialize json file*/
	if (access("/tmp/lights.json", F_OK))
	{
			system("touch /tmp/lights.json");
			system("chmod 666 /tmp/lights.json");
	}

	cJSON *json = OpenJson();
	cJSON *json_lights = cJSON_GetObjectItem(json, "lights");
	if (!json_lights) {
		cout << "ERROR get json first node" << endl;
		CloseJson(json);
		return false;
	}
	int jnum = cJSON_GetArraySize(json_lights);
	for (auto i = 0; i < jnum; i++)
		DeleteJson(json_lights, 0);
  SaveJson(json);
	CloseJson(json);

	return true;
}

void MiWiGtw::routine()
{
	check_json_upda();
	check_offiline_device();
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

void MiWiGtw::check_json_upda()
{
	struct stat state;
	if (!stat(config_file.c_str(), &state)) {
		//cout << "config_timestamp: " << config_timestamp << endl;
		//cout << "state.st_mtime: " << state.st_mtime << endl;
		if (config_timestamp == state.st_mtime)
			return;
	} else {
		cout << "ERROR: stat() " << config_file << endl;
		return;
	}

	cout << "json was updated by web" << endl;
	timer_ptr->stop();

	int array_num = 0;
	int i = 0;
	int webupda = 0;
	cJSON *json, *json_light, *json_array_ori, *json_array, *json_webupda, *json_id, *json_name, *json_valid, *json_led, *json_temp, *json_rssi, *json_clkwise, *json_cntclkwise;

	json = OpenJson();
	json_light = cJSON_GetObjectItem(json, "lights");
	if (!json_light) {
		cerr << "ERROR get json first node" << endl;
		CloseJson(json);
		timer_ptr->start();
		return;
	}

	if (json_light->type != cJSON_Array) {
		cerr << "Error json_light->type=" << json_light->type << endl;
		timer_ptr->start();
		return;
	}

	array_num = cJSON_GetArraySize(json_light);
	if (array_num < 0) {
		cerr << "Error light number:" << array_num << endl;
		timer_ptr->start();
		return;
	} else if (array_num == 0) {
		timer_ptr->start();
		return;
	}

	for (i = 0; i < array_num; i++) {
		json_array_ori = cJSON_GetArrayItem(json_light, i);
		if (!json_array_ori) {
			cerr << "Error cJSON_GetArrayItem:" << i << endl;
			timer_ptr->start();
			return;
		}

		json_webupda         = cJSON_GetObjectItem(json_array_ori, JSON_WEBUPDA);
		json_id         = cJSON_GetObjectItem(json_array_ori, JSON_ID);
		json_name       = cJSON_GetObjectItem(json_array_ori, JSON_NAME);
		json_valid       = cJSON_GetObjectItem(json_array_ori, JSON_VALID);
		json_led        = cJSON_GetObjectItem(json_array_ori, JSON_LED);
		json_temp          = cJSON_GetObjectItem(json_array_ori, JSON_TEMPERATURE);
		json_rssi             = cJSON_GetObjectItem(json_array_ori, JSON_RSSI);
		json_clkwise      = cJSON_GetObjectItem(json_array_ori, JSON_CLKWISE);
		json_cntclkwise = cJSON_GetObjectItem(json_array_ori, JSON_CNTCLKWISE);
		if ((!json_webupda || !json_id || !json_name || !json_valid || !json_led || !json_temp || !json_rssi || !json_clkwise || !json_cntclkwise) || \
				(json_webupda->type != cJSON_String || \
					json_id->type != cJSON_String || \
					json_name->type != cJSON_String || \
					json_valid->type != cJSON_String || \
					json_led->type != cJSON_String || \
					json_temp->type != cJSON_String || \
					json_rssi->type != cJSON_String || \
					json_clkwise->type != cJSON_String || \
					json_cntclkwise->type != cJSON_String)) {
			cerr << "Error get json array items" << endl;
			timer_ptr->start();
			return;
		}

		webupda = std::stoi(json_webupda->valuestring);
		switch (webupda) {
			case 0:
				break;
			case 1:
				lights[i].dev_attr.led = lights[i].dev_attr.led ? false : true;
				if (lights[i].dev_attr.led) {
					m_str_asyn = "send " + std::to_string(i) + " 0 LED1 1" + "\r";
					lights[i].dev_attr.ledstr = "1";
					lights[i].button->checked(false);
				}	else {
					m_str_asyn = "send " + std::to_string(i) + " 0 LED1 0" + "\r";
					lights[i].dev_attr.ledstr = "0";
					lights[i].button->checked(true);
				}
				break;
			case 2:
				lights[i].dev_attr.gpio1 = lights[i].dev_attr.gpio1 ? false : true;
				if (lights[i].dev_attr.gpio1) {
					m_str_asyn = "send " + std::to_string(i) + " 0 GPIO1 1" + "\r";
					lights[i].dev_attr.clkwise = "1";
					lights[i].button1->checked(true);
				}	else {
					m_str_asyn = "send " + std::to_string(i) + " 0 GPIO1 0" + "\r";
					lights[i].dev_attr.clkwise = "0";
					lights[i].button1->checked(false);
				}
				break;
			case 3:
				lights[i].dev_attr.gpio2 = lights[i].dev_attr.gpio2 ? false : true;
				if (lights[i].dev_attr.gpio2) {
					m_str_asyn = "send " + std::to_string(i) + " 0 GPIO2 1" + "\r";
					lights[i].dev_attr.cntclkwise = "1";
					lights[i].button2->checked(true);
				}	else {
					m_str_asyn = "send " + std::to_string(i) + " 0 GPIO2 0" + "\r";
					lights[i].dev_attr.cntclkwise = "0";
					lights[i].button2->checked(false);
				}
				break;
			default:
				cerr << "Error state of webupda: " << webupda << " i: " << i << endl;
				break;
		}

		if (webupda) {
			cout << "web s-> " << m_str_asyn << endl;
			tty_ptr->write(m_str_asyn.data(), m_str_asyn.length());
			break;
		}
	}

	if (webupda) {
		json_array = cJSON_CreateObject();
		if (!json_array) {
			cerr << "ERROR create object json" << endl;
			CloseJson(json);
			timer_ptr->start();
			return;
		}

		if (!cJSON_AddStringToObject(json_array, JSON_WEBUPDA, "0") || \
			!cJSON_AddStringToObject(json_array, JSON_ID, json_id->valuestring) || \
			!cJSON_AddStringToObject(json_array, JSON_VALID, json_valid->valuestring) || \
			!cJSON_AddStringToObject(json_array, JSON_NAME, json_name->valuestring) || \
			!cJSON_AddStringToObject(json_array, JSON_LED, json_led->valuestring) || \
			!cJSON_AddStringToObject(json_array, JSON_TEMPERATURE, json_temp->valuestring) || \
			!cJSON_AddStringToObject(json_array, JSON_RSSI, json_rssi->valuestring) || \
			!cJSON_AddStringToObject(json_array, JSON_CLKWISE, json_clkwise->valuestring) || \
			!cJSON_AddStringToObject(json_array, JSON_CNTCLKWISE, json_cntclkwise->valuestring)) {
			cerr << "ERROR add string to json" << endl;
			CloseJson(json);
			timer_ptr->start();
			return;
		}
		cJSON_ReplaceItemInArray(json_light, i, json_array);
		SaveJson(json);
		clear_text();
		show_text();

	}
	CloseJson(json);
	timer_ptr->start();
}

cJSON* MiWiGtw::OpenJson()
{
	ifstream infile;
	int ret;
	char *text;
	struct stat state;
	cJSON *json = NULL;

	ret = stat(config_file.c_str(), &state);
	if (ret < 0) {
		if (errno == ENOENT) {
			cerr << "json file not exist!" << endl;
		} else
			return NULL;
	}

	text = new char[state.st_size+1];
	if (!text) {
		return NULL;
	}

	infile.open(config_file);
	if (!infile) {
		delete text;
		return NULL;
	}

	infile.read(text, state.st_size);
	if (infile.gcount() != state.st_size) {
		infile.close();
		delete text;
		return NULL;
	}
	text[state.st_size] = '\0';

	json = cJSON_Parse(text);
	if (!json) {
		cout << "blank json, create new..." << endl;
		json = cJSON_CreateObject();
		if (!json) {
			cerr << "cJSON_CreateObject fail" << endl;
			infile.close();
			delete text;
			return NULL;
		}

		if (!cJSON_AddArrayToObject(json, JSON_LIGHTS)) {
			cerr << "cJSON_AddArrayToObject fail" << endl;
			CloseJson(json);
			infile.close();
			delete text;
			return NULL;
		}
	}

	infile.close();
	delete text;
	return json;
}

void MiWiGtw::CloseJson(cJSON *json)
{
	if (json)
		 cJSON_Delete(json);
	return;
}

void MiWiGtw::AddJson(miwi_dev_st dev)
{
	cJSON *json_lights, *json_light;
	std::string id, valid;
	cJSON *json = OpenJson();

	json_lights = cJSON_GetObjectItem(json, "lights");
	if (!json_lights) {
		cout << "ERROR get json first node" << endl;
		CloseJson(json);
		return;
	}

	json_light = cJSON_CreateObject();
	if (!json_light) {
		cout << "ERROR create object json" << endl;
		CloseJson(json);
		return;
	}

	id = std::to_string(light_num + 1);
	valid = std::to_string(dev.valid);

	if (!cJSON_AddStringToObject(json_light, JSON_WEBUPDA, "0") || \
		!cJSON_AddStringToObject(json_light, JSON_ID, id.c_str()) || \
		!cJSON_AddStringToObject(json_light, JSON_VALID, valid.c_str()) || \
		!cJSON_AddStringToObject(json_light, JSON_NAME, dev.name.c_str()) || \
		!cJSON_AddStringToObject(json_light, JSON_LED, dev.ledstr.c_str()) || \
		!cJSON_AddStringToObject(json_light, JSON_TEMPERATURE, dev.temp.c_str()) || \
		!cJSON_AddStringToObject(json_light, JSON_RSSI, dev.rssi.c_str()) || \
		!cJSON_AddStringToObject(json_light, JSON_CLKWISE, dev.clkwise.c_str()) || \
		!cJSON_AddStringToObject(json_light, JSON_CNTCLKWISE, dev.cntclkwise.c_str())) {
		cout << "ERROR add string to json" << endl;
		CloseJson(json);
		return;
	}

	cJSON_AddItemToArray(json_lights, json_light);
	SaveJson(json);
	CloseJson(json);
}

void MiWiGtw::UpdateJson(miwi_dev_st dev)
{
	cJSON *json, *json_light, *json_array;
	int array_num = 0;
	std::string valid;

	json = OpenJson();
	json_light = cJSON_GetObjectItem(json, "lights");
	if (!json_light) {
		cout << "ERROR get json first node" << endl;
		CloseJson(json);
		return;
	}

	array_num = std::stoi(dev.index);
	valid = std::to_string(dev.valid);

	json_array = cJSON_CreateObject();
	if (!json_array) {
		cout << "ERROR create object json" << endl;
		CloseJson(json);
		return;
	}

	if (!cJSON_AddStringToObject(json_array, JSON_WEBUPDA, "0") || \
		!cJSON_AddStringToObject(json_array, JSON_ID, dev.index.c_str()) || \
		!cJSON_AddStringToObject(json_array, JSON_VALID, valid.c_str()) || \
		!cJSON_AddStringToObject(json_array, JSON_NAME, dev.name.c_str()) || \
		!cJSON_AddStringToObject(json_array, JSON_LED, dev.ledstr.c_str()) || \
		!cJSON_AddStringToObject(json_array, JSON_TEMPERATURE, dev.temp.c_str()) || \
		!cJSON_AddStringToObject(json_array, JSON_RSSI, dev.rssi.c_str()) || \
		!cJSON_AddStringToObject(json_array, JSON_CLKWISE, dev.clkwise.c_str()) || \
		!cJSON_AddStringToObject(json_array, JSON_CNTCLKWISE, dev.cntclkwise.c_str())) {
		cout << "ERROR add string to json" << endl;
		CloseJson(json);
		return;
	}

	cJSON_ReplaceItemInArray(json_light, array_num, json_array);
	SaveJson(json);
	CloseJson(json);
}

void MiWiGtw::SaveJson(cJSON *json)
{
	FILE *fp;
	int ret;
	struct stat state;
	if (0 > stat(config_file.c_str(), &state)) {
		cerr << "stat json error" << endl;
		return;
	}

	fp=fopen(config_file.c_str(), "w");
	if (!fp) {
		cerr << "open json fail for saving" << endl;
		return;
	}

	ret = fputs(cJSON_Print(json), fp);
	fclose(fp);
	if (ret < 0) {
		cerr << "save json fail" << endl;
		return;
	}

	stat(config_file.c_str(), &state);
	config_timestamp = state.st_mtime;
}

void MiWiGtw::DeleteJson(cJSON* json_lights, int num)
{
	return cJSON_DeleteItemFromArray(json_lights, num);
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

#ifdef MCHPDEMO
	auto img = make_shared<egt::ImageLabel>(Image("file:mchplogo.png"));
#else
	auto img = make_shared<egt::ImageLabel>(Image("file:logo.png"));
#endif
	img->align(AlignFlag::left);
	img->padding(10);
	window->add(img);

	auto header = make_shared<egt::Label>("Mi-Wi IoT Control Center",
										Rect(Point(0, 35), Size(window->width(), 100)),
										AlignFlag::left);
	header->font(Font(30, Font::Weight::bold));
	window->add(header);

#ifdef MCHPDEMO
	auto title = make_shared<egt::Label>(" ID        Name                 LED        Temperature  RSSI          GPIO1                   GPIO2",
										Rect(Point(0, 100), Size(window->width(), 30)), AlignFlag::left);
#else
	auto title = make_shared<egt::Label>(" ID        Name                 LED        Temperature  RSSI         Forward                 Reverse",
										Rect(Point(0, 100), Size(window->width(), 30)), AlignFlag::left);
#endif

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

	auto timer = make_shared<PeriodicTimer>(std::chrono::milliseconds(50));
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

	std::string addr;
	auto webaddr = make_shared<TextBox>("http://", Rect(Point(280, 20), Size(260, 30)), AlignFlag::center);
	addr = "remote control address: http://" + gateway->get_local_IP() + "/cgi-bin/gateway.cgi";
	webaddr->text(addr);
	window->add(webaddr);

	return app->run();
}
