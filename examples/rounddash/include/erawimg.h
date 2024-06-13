#ifndef __ERAW_DATA_H
#define __ERAW_DATA_H

typedef struct {	
    std::string name;	
    int offset;	
    int len;	
} eraw_st;	
	
eraw_st offset_table[] = {	
    {"Speedo", 0, 884338},    //0	
    {"logobg", 884338, 11958},    //1	
    {"MicrochipLogo", 896296, 17728},    //2	
    {"infobg", 914024, 34},    //3	
    {"Speedo-blue-blur", 914058, 982386},    //4	
    {"StA", 1896444, 1310},    //5	
    {"RtA", 1897754, 836},    //6	
    {"LtA", 1898590, 828},    //7	
    {"destinationIcon", 1899418, 2766},    //8	
};

#endif
