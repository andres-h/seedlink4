#include <stdio.h>
#include <libmseed.h>

static void print_stderr(const char *message) {
	fprintf(stderr, "%s", message);
}

int main() {
	int err;
	MS3Record *msr = NULL;

	ms_loginit (print_stderr, NULL, print_stderr, NULL);

	if ( (err = ms3_readmsr (&msr, "/dev/stdin", NULL, NULL, MSF_UNPACKDATA, 0)) < 0 ) {
		fprintf(stderr, "%s\n", ms_errorstr(err));
		return 1;
	}

	char format;

	if ( msr->formatversion == 2 ) {
		format = '2';

		if ( msr->samprate == 0.0 ) {
			if ( msr->samplecnt != 0 ) {
				if ( msr->sampletype == 'a' )
					format = 'L';  // log
			}
			else if ( mseh_exists(msr, "FDSN.Event.Detection" ) ) {
				format = 'E';
			}
			else if ( mseh_exists(msr, "FDSN.Calibration.Sequence" ) ) {
				format = 'C';
			}
			else if ( mseh_exists(msr, "FDSN.Time.Exception" ) ) {
				format = 'T';
			}
			else {
				format = 'O';  // opaque
			}
		}
	}
	else if ( msr->formatversion == 3 ) {
		format = '3';
	}
	else {
		fprintf(stderr, "unsupported version\n");
		return 1;
	}

	printf("%c", format);
	return 0;
}

