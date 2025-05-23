# test conversion from date tuple to timestamp and back

try:
    import time
except ImportError:
    print("SKIP")
    raise SystemExit

# Test range
MIN_YEAR = 1900
MAX_YEAR = 2099

_MIN_SEC_PER_MONTH = 28 * 86400 - 7200  # incl. DST change
_MAX_SEC_PER_MONTH = 31 * 86400 + 7200  # incl. DST change
_INVALID_TIMESTAMP = -12 * _MAX_SEC_PER_MONTH


# mktime function that checks that the result is reversible
def safe_mktime(tuple):
    try:
        res = time.mktime(tuple)
        chk = time.localtime(res)
    except OverflowError:
        print("safe_mktime:", tuple, "overflow error")
        return _INVALID_TIMESTAMP
    if chk[0:5] != tuple[0:5]:
        print("safe_mktime:", tuple[0:5], " -> ", res, " -> ", chk[0:5])
        return _INVALID_TIMESTAMP
    return res


def test_fwd(date_tuple):
    safe_stamp = safe_mktime(date_tuple)
    year = date_tuple[0]
    month = date_tuple[1] + 1
    if month > 12:
        year += 1
        month = 1
    while year <= MAX_YEAR:
        while month <= 12:
            next_tuple = (year, month) + date_tuple[2:]
            next_stamp = safe_mktime(next_tuple)
            if next_stamp < safe_stamp:
                return date_tuple
            elif next_stamp < safe_stamp + _MIN_SEC_PER_MONTH:
                return date_tuple
            elif next_stamp > safe_stamp + _MAX_SEC_PER_MONTH:
                return date_tuple
            date_tuple = next_tuple
            safe_stamp = next_stamp
            month += 1
        year += 1
        month = 1
    return date_tuple


def test_bwd(date_tuple):
    safe_stamp = safe_mktime(date_tuple)
    year = date_tuple[0]
    month = date_tuple[1] - 1
    if month < 1:
        year -= 1
        month = 12
    while year >= MIN_YEAR:
        while month >= 1:
            next_tuple = (year, month) + date_tuple[2:]
            next_stamp = safe_mktime(next_tuple)
            if next_stamp > safe_stamp:
                return date_tuple
            elif next_stamp > safe_stamp - _MIN_SEC_PER_MONTH:
                return date_tuple
            elif next_stamp < safe_stamp - _MAX_SEC_PER_MONTH:
                return date_tuple
            date_tuple = next_tuple
            safe_stamp = next_stamp
            month -= 1
        year -= 1
        month = 12
    return date_tuple


# start test from Jan 1, 2001 as beginning of 2000 might be broken
start_date = (2001, 1, 15, 0, 0, 0, 0, 0, -1)
large_date = test_fwd(start_date)
small_date = test_bwd(large_date)
print("supported dates: from", small_date[0:2], "to", large_date[0:2])
