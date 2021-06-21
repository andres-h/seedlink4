#include <stdio.h>
#include <libmseed.h>

static void print_stderr(const char *message) {
	fprintf(stderr, "%s", message);
}

static void record_handler(char *record, int reclen, void *handlerdata) {
	fwrite(record, reclen, 1, stdout);
}

int main() {
	int err;
	MS3Record *msr = 0;

	ms_loginit (print_stderr, NULL, print_stderr, NULL);

	while ( (err = ms3_readmsr (&msr, "/dev/stdin", NULL, NULL, MSF_UNPACKDATA, 0)) == MS_NOERROR ) {
		msr->formatversion = 3;
		msr->reclen = -1;

		msr3_pack(msr, record_handler, NULL, NULL, MSF_FLUSHDATA, 0);
	}

	if ( err < 0 ) {
		printf("%s\n", ms_errorstr(err));
		return 1;
	}

	return 0;
}

