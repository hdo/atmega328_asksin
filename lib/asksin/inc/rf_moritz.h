#ifndef _RF_MORITZ_H
#define _RF_MORITZ_H

#define MAX_MORITZ_MSG 30

typedef struct {
    uint8_t length;
    uint8_t message_count;
    uint8_t message_flag;
    uint8_t message_type;
    uint8_t source[3];
    uint8_t destination[3];
    uint8_t group;
    uint8_t payload[MAX_MORITZ_MSG];
} MAX_RF_DATA;

extern uint8_t moritz_on;
extern MAX_RF_DATA max_data;

void rf_moritz_init(void);
void rf_moritz_task(void);
void moritz_func(char *in);
uint8_t rf_moritz_data_available(void);

#endif
