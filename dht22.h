#include <linux/hrtimer.h>

#define GPIO_DEFAULT 6
#define AUTOUPDATE_DEFAULT false

/*
 * 2s minimum between readings. Empirically observed that a small timeout
 * leads to more errors (missed interrupts or hash mismatches)
 */
#define AUTOUPDATE_TIMEOUT_MIN 2000
#define AUTOUPDATE_TIMEOUT_MAX (10 * 60 * 1000) /* 10 minutes */
#define DATA_SIZE 5 /* Number of bytes the DHT22 sensor sends */
#define BITS_PER_BYTE 8
#define EXPECTED_IRQ_COUNT 86 /* The total number of interrupts to process */
#define TRIGGER_IRQ_COUNT 3
#define INIT_RESPONSE_IRQ_COUNT 2
#define DATA_IRQ_COUNT 80

#define MAX_RETRY_COUNT 5
#define RETRY_TIMEOUT 2 /* Seconds */

#define LOW 0
#define HIGH 1

/* signal lengths in ms */
#define TRIGGER_DELAY 100
#define TRIGGER_SIGNAL_LEN 10

/* signal lengths in us */
#define TRIGGER_POST_DELAY 40
#define PREP_SIGNAL_LEN 50

static int setup_dht22_gpio(int gpio);
static int setup_dht22_irq(int gpio);
static void verify_timeout(void);

static void reset_data(void);
static void setup_dht22_timer(struct hrtimer *hres_timer,
			ktime_t delay,
			enum hrtimer_restart (*func)(struct hrtimer *hrtimer));
static void trigger_sensor(struct work_struct *work);
static enum hrtimer_restart timer_func(struct hrtimer *hrtimer);
static enum hrtimer_restart retry_timer_func(struct hrtimer *hrtimer);

static irqreturn_t dht22_irq_handler(int irq, void *data);
static void cleanup_func(struct work_struct *work);
static void process_data(void);
static void process_results(struct work_struct *work);

static ssize_t
gpio_number_show(struct kobject *kobj, struct kobj_attribute *attr, char *buf);

static ssize_t
autoupdate_show(struct kobject *kobj, struct kobj_attribute *attr, char *buf);

static ssize_t
autoupdate_store(struct kobject *kobj,
		struct kobj_attribute *attr,
		const char *buf,
		size_t count);

static ssize_t
autoupdate_timeout_ms_show(struct kobject *kobj,
			struct kobj_attribute *attr,
			char *buf);

static ssize_t
autoupdate_timeout_ms_store(struct kobject *kobj,
			struct kobj_attribute *attr,
			const char *buf,
			size_t count);

static ssize_t
temperature_show(struct kobject *kobj, struct kobj_attribute *attr, char *buf);

static ssize_t
humidity_show(struct kobject *kobj, struct kobj_attribute *attr, char *buf);

static ssize_t
timestamp_show(struct kobject *kobj, struct kobj_attribute *attr, char *buf);

static ssize_t
trigger_store(struct kobject *kobj,
		struct kobj_attribute *attr,
		const char *buf,
		size_t count);
