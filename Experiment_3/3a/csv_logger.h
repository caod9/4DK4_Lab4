#ifndef _CSV_LOGGER_H_
#define _CSV_LOGGER_H_

#include <stdio.h>
#include <stdarg.h>

/* Generic function to log a row of data to any CSV file.
 * Automatically writes headers if the file is empty or new.
 */
void log_row_to_csv(const char *filename, const char *headers, const char *format, ...);

#endif /* _CSV_LOGGER_H_ */