#include <stdint.h>
#include <stdio.h>
#include <stdlib.h>   // exit
#include <string.h>

static const char* const_text_1 = "<script>";
static const char* const_text_2 = "</script>";
static const char* const_text_3 = "var myString = '";
static const char* const_text_4 = "';";
static const char* const_text_5 = "data = JSON.parse(myString); newdata2_resize(data);";
static const char* const_text_6 = "";

void usage() {
  fprintf(stderr, "Usage: unmakeself <input html>\n");
  exit(0);
}

// Function to check if we're in the "events" array
int is_in_events(const char* json, const char* current, const char* events_start) {
  if (events_start == NULL) return 0;
  // Check if current position is after events_start and before the closing bracket
  return (current >= events_start && *current != ']');
}

// Function to write JSON with custom pretty-printing
void write_pretty_json(const char* json, int len, FILE* fout) {
  int indent = 0;
  const int indent_size = 2; // Spaces per indent level
  char prev_char = '\0';
  int in_sublist = 0; // Tracks if we're inside a sublist of "events"
  const char* events_start = NULL; // Position of "events" array start

  // Find the start of "events" array
  const char* events_key = strstr(json, "\"events\"");
  if (events_key) {
    events_start = strchr(events_key, '[');
  }

  for (int i = 0; i < len; i++) {
    char c = json[i];

    // Check if we're entering or exiting a sublist in "events"
    if (is_in_events(json, json + i, events_start)) {
      if (c == '[' && prev_char != '[') { // Start of a sublist
        in_sublist++;
      } else if (c == ']' && in_sublist > 0) { // End of a sublist
        in_sublist--;
      }
    }

    if (c == '{' || c == '[') {
      fputc(c, fout);
      fputc('\n', fout);
      indent += indent_size;
      for (int j = 0; j < indent; j++) fputc(' ', fout);
      prev_char = c;
    } else if (c == '}' || c == ']') {
      fputc('\n', fout);
      indent -= indent_size;
      for (int j = 0; j < indent; j++) fputc(' ', fout);
      fputc(c, fout);
      prev_char = c;
    } else if (c == ',' && !in_sublist && (prev_char != '}' && prev_char != ']')) {
      // Only add newline after comma if not in a sublist
      fputc(c, fout);
      fputc('\n', fout);
      for (int j = 0; j < indent; j++) fputc(' ', fout);
      prev_char = c;
    } else {
      fputc(c, fout);
      prev_char = c;
    }
  }
  fputc('\n', fout); // Ensure file ends with newline
}

int main(int argc, const char** argv) {
  FILE* finhtml;
  if (argc < 2) {
    fprintf(stderr, "Input file xxx.html must be specified.\n");
    usage();
  }

  // Get input filename
  const char* input_file = argv[1];
  finhtml = fopen(input_file, "rb");
  if (finhtml == NULL) {
    fprintf(stderr, "%s did not open.\n", input_file);
    return 0;
  }

  // Create output filename by replacing .html with .json
  char output_file[256];
  strncpy(output_file, input_file, sizeof(output_file) - 1);
  output_file[sizeof(output_file) - 1] = '\0';
  char* dot = strrchr(output_file, '.');
  if (dot == NULL || strcmp(dot, ".html") != 0) {
    fprintf(stderr, "Input file must have .html extension\n");
    fclose(finhtml);
    return 0;
  }
  strcpy(dot, ".json");

  // Open output file
  FILE* foutjson = fopen(output_file, "wb");
  if (foutjson == NULL) {
    fprintf(stderr, "Could not open output file %s\n", output_file);
    fclose(finhtml);
    return 0;
  }

  char* inhtml_buf = new char[250000000]; // 250MB

  int html_len = fread(inhtml_buf, 1, 250000000, finhtml);
  fclose(finhtml);

  char* self0 = strstr(inhtml_buf, "<!-- selfcontained0 -->");
  char* self1 = strstr(inhtml_buf, "<!-- selfcontained1 -->");
  char* self2 = strstr(inhtml_buf, "<!-- selfcontained2 -->");

  if (self0 == NULL || self1 == NULL || self2 == NULL) {
    fprintf(stderr, "%s does not contain selfcontained* comments\n", input_file);
    fclose(foutjson);
    delete[] inhtml_buf;
    exit(0);
  }

  char* self1_end = strchr(self1 + 1, '\n');
  if (self1_end == NULL) {
    fprintf(stderr, "Missing <cr> after selfcontained1\n");
    fclose(foutjson);
    delete[] inhtml_buf;
    exit(0);
  }
  ++self1_end;  // over the <cr>

  char* self2_end = strchr(self2 + 1, '\n');
  if (self2_end == NULL) {
    fprintf(stderr, "Missing <cr> after selfcontained2\n");
    fclose(foutjson);
    delete[] inhtml_buf;
    exit(0);
  }
  ++self2_end;  // over the <cr>

  // JSON is in self1_end .. self_2
  // Within this, there is a single-quote string that we want.
  *self2 = '\0';
  char* quote1 = strchr(self1_end, '\'');
  if (quote1 == NULL) {
    fprintf(stderr, "Missing '..' string\n");
    fclose(foutjson);
    delete[] inhtml_buf;
    return 0;
  }
  ++quote1; // Over the quote

  char* quote2 = strchr(quote1, '\'');
  if (quote2 == NULL) {
    fprintf(stderr, "Missing '..' string\n");
    fclose(foutjson);
    delete[] inhtml_buf;
    return 0;
  }

  // Length of JSON in HTML piece
  int len3 = quote2 - quote1;
  write_pretty_json(quote1, len3, foutjson);

  fclose(foutjson);
  delete[] inhtml_buf;
  return 0;
}