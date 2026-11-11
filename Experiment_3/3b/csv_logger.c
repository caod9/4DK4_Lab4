#include <stdio.h>
#include <stdlib.h>
#include "csv_logger.h"

void log_row_to_csv(const char *filename, const char *headers, const char *format, ...)
{
  FILE *fp;
  va_list args;
  int file_is_empty = 0;

  /* Check if file already exists or is empty */
  fp = fopen(filename, "r");
  if (fp == NULL) {
    file_is_empty = 1;
  } else {
    fseek(fp, 0, SEEK_END);
    if (ftell(fp) == 0) {
      file_is_empty = 1;
    }
    fclose(fp);
  }

  /* Open in append mode */
  fp = fopen(filename, "a");
  if (fp == NULL) {
    printf("Error: Could not open file %s for CSV logging.\n", filename);
    return;
  }

  /* Write headers if it's a new file */
  if (file_is_empty && headers != NULL) {
    fprintf(fp, "%s\n", headers);
  }

  /* Write the formatted data row */
  va_start(args, format);
  vfprintf(fp, format, args);
  va_end(args);

  fprintf(fp, "\n");
  fclose(fp);
}