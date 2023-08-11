/*
 * (C) Copyright 2005- ECMWF.
 *
 * This software is licensed under the terms of the Apache Licence Version 2.0
 * which can be obtained at http://www.apache.org/licenses/LICENSE-2.0.
 *
 * In applying this licence, ECMWF does not waive the privileges and immunities granted to it by
 * virtue of its status as an intergovernmental organisation nor does it submit to any jurisdiction.
 */

#include "grib_api_internal.h"
#include "step_optimizer.h"
#include <stdexcept>
/*
  This is used by make_class.pl

  START_CLASS_DEF
  CLASS      = accessor
  SUPER      = grib_accessor_class_long
  IMPLEMENTS = unpack_long;pack_long
  IMPLEMENTS = init;dump
  MEMBERS = const char* codedStep
  MEMBERS = const char* codedUnits
  MEMBERS = const char* stepUnits
  MEMBERS = const char* indicatorOfUnitForTimeRange
  MEMBERS = const char* lengthOfTimeRange

  END_CLASS_DEF

 */

/* START_CLASS_IMP */

/*

Don't edit anything between START_CLASS_IMP and END_CLASS_IMP
Instead edit values between START_CLASS_DEF and END_CLASS_DEF
or edit "accessor.class" and rerun ./make_class.pl

*/

static int pack_long(grib_accessor*, const long* val, size_t* len);
static int unpack_long(grib_accessor*, long* val, size_t* len);
static int pack_double(grib_accessor*, const double* val, size_t* len);
static int unpack_double(grib_accessor*, double* val, size_t* len);
static int pack_string(grib_accessor*, const char* val, size_t* len);
static int unpack_string(grib_accessor*, char* val, size_t* len);
static void dump(grib_accessor*, grib_dumper*);
static void init(grib_accessor*, const long, grib_arguments*);

typedef struct grib_accessor_step_in_units
{
    grib_accessor att;
    /* Members defined in gen */
    /* Members defined in long */
    /* Members defined in step_in_units */
    const char* codedStep;
    const char* codedUnits;
    const char* stepUnits;
    const char* indicatorOfUnitForTimeRange;
    const char* lengthOfTimeRange;
} grib_accessor_step_in_units;

extern grib_accessor_class* grib_accessor_class_long;

static grib_accessor_class _grib_accessor_class_step_in_units = {
    &grib_accessor_class_long,                      /* super */
    "step_in_units",                      /* name */
    sizeof(grib_accessor_step_in_units),  /* size */
    0,                           /* inited */
    0,                           /* init_class */
    &init,                       /* init */
    0,                  /* post_init */
    0,                    /* destroy */
    &dump,                       /* dump */
    0,                /* next_offset */
    0,              /* get length of string */
    0,                /* get number of values */
    0,                 /* get number of bytes */
    0,                /* get offset to bytes */
    0,            /* get native type */
    0,                /* get sub_section */
    0,               /* pack_missing */
    0,                 /* is_missing */
    &pack_long,                  /* pack_long */
    &unpack_long,                /* unpack_long */
    0, //&pack_double,                /* pack_double */
    0,                 /* pack_float */
    0, //&unpack_double,              /* unpack_double */
    0,               /* unpack_float */
    0, //&pack_string,                /* pack_string */
    0, //&unpack_string,              /* unpack_string */
    0,          /* pack_string_array */
    0,        /* unpack_string_array */
    0,                 /* pack_bytes */
    0,               /* unpack_bytes */
    0,            /* pack_expression */
    0,              /* notify_change */
    0,                /* update_size */
    0,             /* preferred_size */
    0,                     /* resize */
    0,      /* nearest_smaller_value */
    0,                       /* next accessor */
    0,                    /* compare vs. another accessor */
    0,      /* unpack only ith value (double) */
    0,       /* unpack only ith value (float) */
    0,  /* unpack a given set of elements (double) */
    0,   /* unpack a given set of elements (float) */
    0,     /* unpack a subarray */
    0,                      /* clear */
    0,                 /* clone accessor */
};


grib_accessor_class* grib_accessor_class_step_in_units = &_grib_accessor_class_step_in_units;

/* END_CLASS_IMP */

static void init(grib_accessor* a, const long l, grib_arguments* c)
{
    grib_accessor_step_in_units* self = (grib_accessor_step_in_units*)a;
    int n                             = 0;

    self->codedStep                   = grib_arguments_get_name(grib_handle_of_accessor(a), c, n++);
    self->codedUnits                  = grib_arguments_get_name(grib_handle_of_accessor(a), c, n++);
    self->stepUnits                   = grib_arguments_get_name(grib_handle_of_accessor(a), c, n++);
    self->indicatorOfUnitForTimeRange = grib_arguments_get_name(grib_handle_of_accessor(a), c, n++);
    self->lengthOfTimeRange           = grib_arguments_get_name(grib_handle_of_accessor(a), c, n++);
}

static void dump(grib_accessor* a, grib_dumper* dumper)
{
    grib_dump_double(dumper, a, NULL);
}

/* Conversion of to seconds - Grib edition 2 table 4.4 */
static const int u2s2[] = {
    60,      /* (0)  minutes   */
    3600,    /* (1)  hour      */
    86400,   /* (2)  day       */
    2592000, /* (3)  month     */
    -1,      /* (4)  year      */
    -1,      /* (5)  decade    */
    -1,      /* (6)  30 years  */
    -1,      /* (7)  century   */
    -1,      /* (8)  RESERVED  */
    -1,      /* (9)  RESERVED  */
    10800,   /* (10) 3 hours   */
    21600,   /* (11) 6 hours   */
    43200,   /* (12) 12 hours  */
    1        /* (13) seconds   */
};

/* Note: 'stepUnits' has a different table with extra entries e.g. 15 and 30 mins */
static const int u2s[] = {
    60,      /* (0)  minutes    */
    3600,    /* (1)  hour       */
    86400,   /* (2)  day        */
    2592000, /* (3)  month      */
    -1,      /* (4)  year       */
    -1,      /* (5)  decade     */
    -1,      /* (6)  30 years   */
    -1,      /* (7)  century    */
    -1,      /* (8)             */
    -1,      /* (9)             */
    10800,   /* (10) 3 hours    */
    21600,   /* (11) 6 hours    */
    43200,   /* (12) 12 hours   */
    1,       /* (13) seconds    */
    900,     /* (14) 15 minutes */
    1800     /* (15) 30 minutes */
};

static int unpack_long(grib_accessor* a, long* val, size_t* len)
{
    grib_accessor_step_in_units* self = (grib_accessor_step_in_units*)a;
    int err                           = 0;
    long codedStep, codedUnits, stepUnits;
    grib_handle* h = grib_handle_of_accessor(a);
    int factor     = 0;
    long u2sf, u2sf_step_unit;


    if ((err = grib_get_long_internal(h, self->codedUnits, &codedUnits)))
        return err;
    if ((err = grib_get_long_internal(h, self->stepUnits, &stepUnits)))
        return err;
    if ((err = grib_get_long_internal(h, self->codedStep, &codedStep)))
        return err;

    if (stepUnits != codedUnits) {
        *val = codedStep * u2s2[codedUnits];
        if (*val < 0) {
            factor = 60;
            if (u2s2[codedUnits] % factor)
                return GRIB_DECODING_ERROR;
            if (u2s[stepUnits] % factor)
                return GRIB_DECODING_ERROR;
            u2sf           = u2s2[codedUnits] / factor;
            *val           = codedStep * u2sf;
            u2sf_step_unit = u2s[stepUnits] / factor;
        }
        else {
            u2sf_step_unit = u2s[stepUnits];
        }

        if (*val % u2sf_step_unit != 0) {
            err  = grib_set_long_internal(h, self->stepUnits, codedUnits);
            *val = codedStep;
            return err;
        }
        *val = *val / u2sf_step_unit;
    }
    else
        *val = codedStep;

    return GRIB_SUCCESS;
}

static int pack_long(grib_accessor* a, const long* val, size_t* len)
{
    grib_accessor_step_in_units* self = (grib_accessor_step_in_units*)a;
    grib_handle* h                   = grib_handle_of_accessor(a);
    int ret = 0;

    Step step{(int) *val, StepUnitsTable::to_long("h")};
    ret = grib_set_long_internal(h, "indicatorOfUnitOfTimeRange", step.unit_as_long());
    if (ret)
        return ret;

    ret = grib_set_long_internal(h, "forecastTime", step.value());
    if (ret)
        return ret;
    return GRIB_SUCCESS;

    //grib_accessor_step_in_units* self = (grib_accessor_step_in_units*)a;
    //grib_handle* h                    = grib_handle_of_accessor(a);
    //int err                           = 0;
    //long codedStep, codedUnits, stepUnits;
    //long oldStep = 0;
    //long indicatorOfUnitForTimeRange, lengthOfTimeRange;

    //if ((err = grib_get_long_internal(h, self->codedUnits, &codedUnits)))
    //    return err;
    //if ((err = grib_get_long_internal(h, self->stepUnits, &stepUnits)))
    //    return err;

    //unpack_long(a, &oldStep, len);

    //if (stepUnits != codedUnits) {
    //    codedStep = *val * u2s[stepUnits];
    //    if (codedStep % u2s2[codedUnits] != 0) {
    //        codedUnits = stepUnits;
    //        err        = grib_set_long_internal(h, self->codedUnits, codedUnits);
    //        if (err != GRIB_SUCCESS)
    //            return err;
    //        codedStep = *val;
    //    }
    //    else {
    //        codedStep = codedStep / u2s2[codedUnits];
    //    }
    //}
    //else {
    //    codedStep = *val;
    //}

    //if (self->indicatorOfUnitForTimeRange) {
    //    if ((err = grib_get_long_internal(h,
    //                                      self->indicatorOfUnitForTimeRange, &indicatorOfUnitForTimeRange)))
    //        return err;
    //    if ((err = grib_get_long_internal(h,
    //                                      self->lengthOfTimeRange, &lengthOfTimeRange)))
    //        return err;
    //    if (codedUnits == indicatorOfUnitForTimeRange)
    //        lengthOfTimeRange -= codedStep - oldStep;
    //    else
    //        lengthOfTimeRange -= codedStep * u2s2[codedUnits] / u2s2[indicatorOfUnitForTimeRange];
    //    lengthOfTimeRange = lengthOfTimeRange > 0 ? lengthOfTimeRange : 0;
    //    err               = grib_set_long_internal(grib_handle_of_accessor(a), self->lengthOfTimeRange, lengthOfTimeRange);
    //    if (err != GRIB_SUCCESS)
    //        return err;
    //}

    //return grib_set_long_internal(grib_handle_of_accessor(a), self->codedStep, codedStep);
}

static int pack_string(grib_accessor* a, const char* val, size_t* len) {
    grib_accessor_step_in_units* self = (grib_accessor_step_in_units*)a;
    grib_handle* h                   = grib_handle_of_accessor(a);
    int ret = 0;

    Step step = Step(parse_step(std::string(val)));
    ret = grib_set_long_internal(h, "indicatorOfUnitOfTimeRange", step.unit_as_long());
    if (ret)
        return ret;

    ret = grib_set_long_internal(h, "forecastTime", step.value());
    if (ret)
        return ret;

    return GRIB_SUCCESS;
}

static int unpack_string(grib_accessor* a, char* val, size_t* len) {
    grib_accessor_step_in_units* self = (grib_accessor_step_in_units*)a;
    grib_handle* h                   = grib_handle_of_accessor(a);
    int ret = 0;

    size_t stepOutputFormatSize = 128;
    char stepOutputFormat[stepOutputFormatSize];
    if ((ret = grib_get_string_internal(h, "stepOutputFormat", stepOutputFormat, &stepOutputFormatSize)) != GRIB_SUCCESS)
        return ret;

    long unit;
    ret = grib_get_long_internal(h, "indicatorOfUnitOfTimeRange", &unit);
    if (ret)
        return ret;

    long value;
    ret = grib_get_long_internal(h, "forecastTime", &value);
    if (ret)
        return ret;

    Step step{(int) value, unit};
    if (strcmp(stepOutputFormat, "future") == 0) {
        sprintf(val, "%d%s", step.value(), step.unit_as_str().c_str());
    }
    else {
        sprintf(val, "%d", step.value());
    }

    return GRIB_SUCCESS;
}


static int pack_double(grib_accessor* a, const double* val, size_t* len) {
    grib_accessor_step_in_units* self = (grib_accessor_step_in_units*)a;
    grib_handle* h                   = grib_handle_of_accessor(a);
    int ret = 0;

    long unit;
    ret = grib_get_long_internal(h, "indicatorOfUnitOfTimeRange", &unit);
    if (ret)
        return ret;


    long stepUnits;
    ret = grib_get_long_internal(h, self->stepUnits, &stepUnits);
    if (ret)
        return ret;

    Step step;
    if (stepUnits != 255) {
        step = Step((int) *val, stepUnits);
    }
    else {
        step = Step((int) *val, unit);
    }

    ret = grib_set_long_internal(h, "forecastTime", step.value());
    if (ret)
        return ret;
    ret = grib_set_long_internal(h, "indicatorOfUnitOfTimeRange", step.unit_as_long());
    if (ret)
        return ret;

    return ret;
}


static int unpack_double(grib_accessor* a, double* val, size_t* len) {
    grib_accessor_step_in_units* self = (grib_accessor_step_in_units*)a;
    grib_handle* h                   = grib_handle_of_accessor(a);
    int ret = 0;

    long unit;
    ret = grib_get_long_internal(h, "indicatorOfUnitOfTimeRange", &unit);
    if (ret)
        return ret;

    long value;
    ret = grib_get_long_internal(h, "forecastTime", &value);
    if (ret)
        return ret;

    long stepUnits;
    ret = grib_get_long_internal(h, self->stepUnits, &stepUnits);
    if (ret)
        return ret;

    if (stepUnits != 255) {
        Step step = Step(value, unit);
        *val = step.getDoubleValue(stepUnits);
    }
    else {
        Step step = Step(value, unit);
        *val = step.value();
    }

    return GRIB_SUCCESS;
}
