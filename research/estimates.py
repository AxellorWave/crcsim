import math

TARGET = 1e-6


def poisson_cumulative(events, mean):
    return sum(math.exp(-mean + k * math.log(mean) - math.lgamma(k + 1)) for k in range(events + 1))


def poisson_upper_bound(events):
    if events == 0:
        return 3.0
    low, high = float(events), events + 10.0 * math.sqrt(events + 1.0) + 10.0
    for _ in range(200):
        middle = 0.5 * (low + high)
        if poisson_cumulative(events, middle) > 0.05:
            low = middle
        else:
            high = middle
    return 0.5 * (low + high)


def upper_bound(events, trials):
    return poisson_upper_bound(events) / trials


def verdict(events, trials):
    if upper_bound(events, trials) <= TARGET:
        return "yes"
    if events / trials > TARGET:
        return "no"
    return "borderline"
