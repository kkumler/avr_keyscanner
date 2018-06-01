
#include <vector>

#include <cstdio>
#include <cstdlib>
#include <cstring>
#include <cassert>
#include <getopt.h>
#include <ctype.h>

// regexp are cool, but takes too much time to compile !
//#define USE_STD_REGEX

#ifdef USE_STD_REGEX
#   include <regex>
#endif

#define SEPARATOR   "########################################"

// stdout goes to run_tests.pl
#define log(__fmt, ...)     do { printf(__fmt "\n", ##__VA_ARGS__); deb("# LOG: " __fmt, ##__VA_ARGS__); } while (0)

// stderr for error and debugging (not catched by run_tests.pl)
#define err(__fmt, ...)     do { fprintf(stderr, "ERROR %s: " __fmt "\n", g_debouncer_name, ##__VA_ARGS__); } while (0)
#define deb(__fmt, ...)     do { if (g_debug) fprintf(stderr, __fmt "\n", ##__VA_ARGS__); } while (0)

const char      *g_debouncer_name = nullptr;
bool            g_debug = false;

// Parses and test one input test data file
class Tester {
  public:
    int         _target_sampling_rate = 625;
    bool        _success = false;

    // Parses filepath data file, then runs the debouncer test
    // Test result in `_success`.
    // Returns false only if test could not be run.
    bool        run_file(const char *filepath) {
        deb("# Running %s %s", g_debouncer_name, filepath);

        _test_name = filepath;

        if (!_parse_file(filepath))
            return false;

        if (_raw_data.size() == 0) {
            err("no data");
            return false;
        }
        if (_data_presses < 0) {
            err("could not find presses number");
            return false;
        }
        if (_data_sampling_rate <= 0) {
            err("invalid sampling rate");
            return false;
        }

        deb(SEPARATOR SEPARATOR);
        deb("# test file sampling rate: %d, simulating sampling rate: %d", _data_sampling_rate, _target_sampling_rate);
        deb("# here, 10 sample = %.2f ms, 10ms = %.2f samples", 1000.0 / double(_target_sampling_rate), double(_target_sampling_rate) / 100.0);

        _success = _run_test();

        deb(SEPARATOR);
        deb("# Final test result: %s", _success ? "SUCCESS" : "FAILURE");

        return true;
    }

  private:
    //
    // Parsing
    //

    const char              *_test_name = nullptr;
    int                     _data_sampling_rate = 625;
    int                     _data_presses = -1;
    std::vector<bool>       _raw_data;

    // Parses filepath input data file
    bool        _parse_file(const char *filepath) {
        FILE    *f = fopen(filepath, "r");
        if (f == nullptr) {
            perror("open data file");
            return false;
        }
        char    *line = nullptr;
        size_t  alloclen = 0;
        ssize_t nread = 0;
        int     linenum = 1;
        for (; (nread = getline(&line, &alloclen, f)) > 0; ++linenum) {
            _parse_line(line, nread, linenum);
        }
        free(line);
        fclose(f);
        return true;
    }

    // Parses a single line of input data file
    void    _parse_line(const char *line, size_t len, int linenum) {
        for (size_t i = 0; i < len; ++i) {
            switch (line[i]) {
            case '0':
                _raw_data.push_back(0);
                break;
            case '1':
                _raw_data.push_back(1);
                break;
            case '\r': // fallthrough
            case '\n':
            case '\t':
            case ' ':
                // ignore
                break;
            case '#':
                _parse_comment(line, i, len, linenum);
                return;
            default:
                deb("# Unkown char %c at line %d", line[i], linenum);
                break;
            }
        }
    }

#if !defined(USE_STD_REGEX)
    // Parses a comment starting at `line + start`
    void    _parse_comment(const char *line, size_t start, size_t end [[maybe_unused]], int linenum [[maybe_unused]]) {
        //deb("# Comment at line %d: %s", linenum, line + start);

        // skip '#'
        while (line[start] == '#')
            ++start;
        // skip spaces
        while (isspace(line[start]))
            ++start;

        static const char   s_sampling_rate[] = "SAMPLES-PER-SECOND:";
        static const char   s_presses[] = "PRESSES:";
        if (strncmp(line + start, s_sampling_rate, sizeof(s_sampling_rate) - 1) == 0) {
            // atoi ignores leading spaces and trailing non-numerical
            _data_sampling_rate = atoi(line + start + sizeof(s_sampling_rate) - 1);
            deb("# found sampling rate %d (target=%d)", _data_sampling_rate, _target_sampling_rate);
        } else if (strncmp(line + start, s_presses, sizeof(s_presses) - 1) == 0) {
            // atoi ignores leading spaces and trailing non-numerical
            _data_presses = atoi(line + start + sizeof(s_presses) - 1);
            deb("# found presses %d", _data_presses);
        }
    }
#else
    // Parses a comment starting at `line + start`
    std::match_results<const char*>     _match; // cache
    void    _parse_comment(const char *line, size_t start, size_t end, int linenum [[maybe_unused]]) {
        //deb("comment at line %d: %s", linenum, line + start);
        static std::regex   s_reg_sampling_rate{"#\\s*SAMPLES-PER-SECOND:\\s*(\\d*)"};
        static std::regex   s_reg_presses{"#\\s*PRESSES:\\s*(\\d*)"};
        if (std::regex_search(line + start, line + end, _match, s_reg_sampling_rate)) {
            _data_sampling_rate = atoi(_match[1].first()); // atoi ignores leading spaces and trailing non-numerical
            deb("# found sampling rate %d (target=%d)", _data_sampling_rate, _target_sampling_rate);
        } else if (std::regex_search(line + start, line + end, _match, s_reg_presses)) {
            _data_presses = atoi(_match[1].first()); // atoi ignores leading spaces and trailing non-numerical
            deb("# found presses %d", _data_presses);
        }
    }
#endif

  private:
    //
    // Testing
    //

    debounce_t  _db;
    int         _presses = 0;
    int         _releases = 0;
    bool        _test_failed = false;

    // Tests the debouncer with parsed data
    bool        _run_test() {
        bzero(&_db, sizeof(_db));

        const int   target_count = _target_sampling_rate * _raw_data.size() / _data_sampling_rate;

        int         total_run_count = 0;

        // Run 'nearest' sample test with 'jitter': give to the debouncer the
        // nearest test sample. re-run with a sampling offset for all possible sampling offsets (jitter).
        {
            int     jitter = 0;
            if (_data_sampling_rate > _target_sampling_rate)
                jitter = 1 + (_data_sampling_rate - 1) / _target_sampling_rate;

            for (int offset = 0; offset <= jitter; ++offset) {
                deb(SEPARATOR);
                deb("# Running test jitter, offset = %d", offset);

                _run_debounce_sample_times(0, 100);
                for (int i = 0; i < target_count - 1; ++i) {
                    int         di = i * _data_sampling_rate / _target_sampling_rate;
                    di += offset;
                    assert(di < int(_raw_data.size()));
                    uint8_t     sample = _raw_data[di];
                    _run_debouce(sample);
                }
                _run_debounce_sample_times(0, 200);

                ++total_run_count;
                if (!_test_failed && _presses != _releases) {
                    log("%s;%s;press_rel_mismatched", g_debouncer_name, _test_name);
                    _test_failed = true;
                }

                deb("# End test: %d presss, %d releases, (%d target)", _presses, _releases, _data_presses * total_run_count);
            }
        }

        // Run test averaging test's samples (if worth at least 2 times more data)
        if (_data_sampling_rate / _target_sampling_rate > 1) {
            // percent
            const int   sample_avg_thresholds[] = { 33, 50, 66 };

            for (int thi = 0; thi < int(sizeof(sample_avg_thresholds) / sizeof(*sample_avg_thresholds)); ++thi) {
                const int   sample_avg_threshold = sample_avg_thresholds[thi];

                deb(SEPARATOR);
                deb("# Running test average > %d%%", sample_avg_threshold);

                int         last_di = 0;
                _run_debounce_sample_times(0, 100);
                for (int i = 0; i < target_count; ++i) {
                    int         di = i * _data_sampling_rate / _target_sampling_rate;
                    assert(di < int(_raw_data.size()));
                    int         samples_sum = _raw_data[di];
                    int         samples_count = 1;
                    for (int j = last_di + 1; j < di; ++j) {
                        samples_sum += _raw_data[j];
                        ++samples_count;
                    }
                    last_di = di;
                    bool        sample_past_threshold = samples_sum * 100 >= sample_avg_threshold * samples_count;

                    uint8_t     sample = sample_past_threshold ? 1 : 0;
                    _run_debouce(sample);
                }
                _run_debounce_sample_times(0, 200);

                ++total_run_count;
                if (!_test_failed && _presses != _releases) {
                    log("%s;%s;press_rel_mismatched", g_debouncer_name, _test_name);
                    _test_failed = true;
                }

                deb("# End test: %d presss, %d releases, (%d target)", _presses, _releases, _data_presses * total_run_count);
            }
        }

        int target_presses = _data_presses * total_run_count;

        if (_presses != target_presses) {
            log("%s;%s;%+.2f", g_debouncer_name, _test_name, double(_presses - target_presses) / double(total_run_count));
            _test_failed = true;
        } else
            log("%s;%s;0", g_debouncer_name, _test_name);

        return !_test_failed;
    }

    int         _last_state = 0;
    int         _out_sample_i = 0;

    // Runs a single call to debounce()
    void        _run_debouce(uint8_t sample) {
        uint8_t     debounced_changes;
        debounced_changes = debounce(sample, &_db);
        bool        overlflow = ((debounced_changes | _db.state) & ~1) != 0;
        if (overlflow) {
            log("%s;%s;overflow", g_debouncer_name, _test_name);
            _test_failed = true;
        }
        bool        said_changed = debounced_changes != 0;
        bool        state_changed = _db.state != _last_state;
        _last_state = _db.state;
        if (said_changed != state_changed) {
            log("%s;%s;changes_miss", g_debouncer_name, _test_name);
            _test_failed = true;
        }
        if (said_changed) {
            if (_db.state)
                ++_presses;
            else
                ++_releases;
        }

        deb("%d %d", sample, _db.state);
        ++_out_sample_i;
        if (_out_sample_i % 10 == 0)
            deb("");
    }

    // Runs `sample` sample `count` times
    void    _run_debounce_sample_times(uint8_t sample, int count) {
        deb("# begin '%d' x %d", sample, count);
        for (int i = 0; i < count; ++i)
            _run_debouce(sample);
        deb("# end '%d' x %d", sample, count);
    };
};

int main(int argc, char *argv[]) {

    g_debouncer_name = argv[0];

    const char      usage[] =
        "usage: %s [-d] [-i interval] data/file/path...\n\
    -i interval     : force a KEYSCAN_INTERVAL\n\
    -d              : enable debug output on stderr\n\
";

    //int       interval = KEYSCAN_INTERVAL_DEFAULT;
    int         interval = 14;

    int         opt;
    while ((opt = getopt(argc, argv, "di:")) != -1) {
        switch (opt) {
        case 'i':
            interval = atoi(optarg);
            break;
        case 'd':
            g_debug = true;
            break;
        default: /* '?' */
            fprintf(stderr, usage, argv[0]);
            exit(1);
        }
    }

    if (optind >= argc) {
        fprintf(stderr, "no input file specified\n");
        exit(1);
    }

    uint64_t    f_cpu = F_CPU;
    uint64_t    prescaler = 256;
    uint64_t    target_sampling_rate = 2000;
    if (interval != 14) {
	target_sampling_rate = f_cpu / (prescaler * uint64_t(interval));
    }

    int         test_sucess = 0;
    int         total_tests = 0;

    for (int i = optind; i < argc; ++i) {
        const char      *f = argv[i];

        deb("Running %s %s", g_debouncer_name, f);
        Tester      t;
        t._target_sampling_rate = target_sampling_rate;
        if (!t.run_file(f)) {
            err("!!! Failed to run the test %s !!!", f);
        } else
            test_sucess += t._success;
        ++total_tests;
    }

    deb("Result %d/%d", test_sucess, total_tests);

}
