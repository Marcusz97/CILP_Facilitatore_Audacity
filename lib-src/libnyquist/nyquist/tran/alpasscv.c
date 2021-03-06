#include "stdio.h"
#ifndef mips
#include "stdlib.h"
#endif
#include "xlisp.h"
#include "sound.h"

#include "falloc.h"
#include "cext.h"
#include "alpasscv.h"

void alpasscv_free();


typedef struct alpasscv_susp_struct {
    snd_susp_node susp;
    long terminate_cnt;
    sound_type input;
    long input_cnt;
    sample_block_values_type input_ptr;
    sound_type feedback;
    long feedback_cnt;
    sample_block_values_type feedback_ptr;

    long delaylen;
    sample_type *delaybuf;
    sample_type *delayptr;
    sample_type *endptr;
} alpasscv_susp_node, *alpasscv_susp_type;


void alpasscv_nn_fetch(register alpasscv_susp_type susp, snd_list_type snd_list)
{
    int cnt = 0; /* how many samples computed */
    int togo;
    int n;
    sample_block_type out;
    register sample_block_values_type out_ptr;

    register sample_block_values_type out_ptr_reg;

    register sample_type * delayptr_reg;
    register sample_type * endptr_reg;
    register sample_block_values_type feedback_ptr_reg;
    register sample_block_values_type input_ptr_reg;
    falloc_sample_block(out, "alpasscv_nn_fetch");
    out_ptr = out->samples;
    snd_list->block = out;

    while (cnt < max_sample_block_len) { /* outer loop */
	/* first compute how many samples to generate in inner loop: */
	/* don't overflow the output sample block: */
	togo = max_sample_block_len - cnt;

	/* don't run past the input input sample block: */
	susp_check_term_samples(input, input_ptr, input_cnt);
	togo = min(togo, susp->input_cnt);

	/* don't run past the feedback input sample block: */
	susp_check_samples(feedback, feedback_ptr, feedback_cnt);
	togo = min(togo, susp->feedback_cnt);

	/* don't run past terminate time */
	if (susp->terminate_cnt != UNKNOWN &&
	    susp->terminate_cnt <= susp->susp.current + cnt + togo) {
	    togo = susp->terminate_cnt - (susp->susp.current + cnt);
	    if (togo == 0) break;
	}

	n = togo;
	delayptr_reg = susp->delayptr;
	endptr_reg = susp->endptr;
	feedback_ptr_reg = susp->feedback_ptr;
	input_ptr_reg = susp->input_ptr;
	out_ptr_reg = out_ptr;
	if (n) do { /* the inner sample computation loop */
register sample_type y, z, fb;
	    y = *delayptr_reg;
        *delayptr_reg++ = z = (sample_type) ((fb = *feedback_ptr_reg++) * y + *input_ptr_reg++);
        *out_ptr_reg++ = (sample_type) (y - fb * z);
        if (delayptr_reg >= endptr_reg) delayptr_reg = susp->delaybuf;;
	} while (--n); /* inner loop */

	susp->delayptr = delayptr_reg;
	/* using feedback_ptr_reg is a bad idea on RS/6000: */
	susp->feedback_ptr += togo;
	/* using input_ptr_reg is a bad idea on RS/6000: */
	susp->input_ptr += togo;
	out_ptr += togo;
	susp_took(input_cnt, togo);
	susp_took(feedback_cnt, togo);
	cnt += togo;
    } /* outer loop */

    /* test for termination */
    if (togo == 0 && cnt == 0) {
	snd_list_terminate(snd_list);
    } else {
	snd_list->block_len = cnt;
	susp->susp.current += cnt;
    }
} /* alpasscv_nn_fetch */


void alpasscv_ns_fetch(register alpasscv_susp_type susp, snd_list_type snd_list)
{
    int cnt = 0; /* how many samples computed */
    int togo;
    int n;
    sample_block_type out;
    register sample_block_values_type out_ptr;

    register sample_block_values_type out_ptr_reg;

    register sample_type * delayptr_reg;
    register sample_type * endptr_reg;
    register sample_type feedback_scale_reg = susp->feedback->scale;
    register sample_block_values_type feedback_ptr_reg;
    register sample_block_values_type input_ptr_reg;
    falloc_sample_block(out, "alpasscv_ns_fetch");
    out_ptr = out->samples;
    snd_list->block = out;

    while (cnt < max_sample_block_len) { /* outer loop */
	/* first compute how many samples to generate in inner loop: */
	/* don't overflow the output sample block: */
	togo = max_sample_block_len - cnt;

	/* don't run past the input input sample block: */
	susp_check_term_samples(input, input_ptr, input_cnt);
	togo = min(togo, susp->input_cnt);

	/* don't run past the feedback input sample block: */
	susp_check_samples(feedback, feedback_ptr, feedback_cnt);
	togo = min(togo, susp->feedback_cnt);

	/* don't run past terminate time */
	if (susp->terminate_cnt != UNKNOWN &&
	    susp->terminate_cnt <= susp->susp.current + cnt + togo) {
	    togo = susp->terminate_cnt - (susp->susp.current + cnt);
	    if (togo == 0) break;
	}

	n = togo;
	delayptr_reg = susp->delayptr;
	endptr_reg = susp->endptr;
	feedback_ptr_reg = susp->feedback_ptr;
	input_ptr_reg = susp->input_ptr;
	out_ptr_reg = out_ptr;
	if (n) do { /* the inner sample computation loop */
register sample_type y, z, fb;
	    y = *delayptr_reg;
        *delayptr_reg++ = z = (sample_type) ((fb = (feedback_scale_reg * *feedback_ptr_reg++)) * y + *input_ptr_reg++);
        *out_ptr_reg++ = (sample_type) (y - fb * z);
        if (delayptr_reg >= endptr_reg) delayptr_reg = susp->delaybuf;;
	} while (--n); /* inner loop */

	susp->delayptr = delayptr_reg;
	/* using feedback_ptr_reg is a bad idea on RS/6000: */
	susp->feedback_ptr += togo;
	/* using input_ptr_reg is a bad idea on RS/6000: */
	susp->input_ptr += togo;
	out_ptr += togo;
	susp_took(input_cnt, togo);
	susp_took(feedback_cnt, togo);
	cnt += togo;
    } /* outer loop */

    /* test for termination */
    if (togo == 0 && cnt == 0) {
	snd_list_terminate(snd_list);
    } else {
	snd_list->block_len = cnt;
	susp->susp.current += cnt;
    }
} /* alpasscv_ns_fetch */


void alpasscv_toss_fetch(susp, snd_list)
  register alpasscv_susp_type susp;
  snd_list_type snd_list;
{
    long final_count = susp->susp.toss_cnt;
    time_type final_time = susp->susp.t0;
    long n;

    /* fetch samples from input up to final_time for this block of zeros */
    while ((round((final_time - susp->input->t0) * susp->input->sr)) >=
	   susp->input->current)
	susp_get_samples(input, input_ptr, input_cnt);
    /* fetch samples from feedback up to final_time for this block of zeros */
    while ((round((final_time - susp->feedback->t0) * susp->feedback->sr)) >=
	   susp->feedback->current)
	susp_get_samples(feedback, feedback_ptr, feedback_cnt);
    /* convert to normal processing when we hit final_count */
    /* we want each signal positioned at final_time */
    n = round((final_time - susp->input->t0) * susp->input->sr -
         (susp->input->current - susp->input_cnt));
    susp->input_ptr += n;
    susp_took(input_cnt, n);
    n = round((final_time - susp->feedback->t0) * susp->feedback->sr -
         (susp->feedback->current - susp->feedback_cnt));
    susp->feedback_ptr += n;
    susp_took(feedback_cnt, n);
    susp->susp.fetch = susp->susp.keep_fetch;
    (*(susp->susp.fetch))(susp, snd_list);
}


void alpasscv_mark(alpasscv_susp_type susp)
{
    sound_xlmark(susp->input);
    sound_xlmark(susp->feedback);
}


void alpasscv_free(alpasscv_susp_type susp)
{
free(susp->delaybuf);    sound_unref(susp->input);
    sound_unref(susp->feedback);
    ffree_generic(susp, sizeof(alpasscv_susp_node), "alpasscv_free");
}


void alpasscv_print_tree(alpasscv_susp_type susp, int n)
{
    indent(n);
    stdputstr("input:");
    sound_print_tree_1(susp->input, n);

    indent(n);
    stdputstr("feedback:");
    sound_print_tree_1(susp->feedback, n);
}


sound_type snd_make_alpasscv(sound_type input, time_type delay, sound_type feedback)
{
    register alpasscv_susp_type susp;
    rate_type sr = max(input->sr, feedback->sr);
    time_type t0 = max(input->t0, feedback->t0);
    int interp_desc = 0;
    sample_type scale_factor = 1.0F;
    time_type t0_min = t0;
    /* combine scale factors of linear inputs (INPUT) */
    scale_factor *= input->scale;
    input->scale = 1.0F;

    /* try to push scale_factor back to a low sr input */
    if (input->sr < sr) { input->scale = scale_factor; scale_factor = 1.0F; }

    falloc_generic(susp, alpasscv_susp_node, "snd_make_alpasscv");
    susp->delaylen = max(1, round(input->sr * delay));
    susp->delaybuf = (sample_type *) calloc (susp->delaylen, sizeof(sample_type));
    susp->delayptr = susp->delaybuf;
    susp->endptr = susp->delaybuf + susp->delaylen;

    /* select a susp fn based on sample rates */
    interp_desc = (interp_desc << 2) + interp_style(input, sr);
    interp_desc = (interp_desc << 2) + interp_style(feedback, sr);
    switch (interp_desc) {
      case INTERP_nn: susp->susp.fetch = alpasscv_nn_fetch; break;
      case INTERP_ns: susp->susp.fetch = alpasscv_ns_fetch; break;
      default: snd_badsr(); break;
    }

    susp->terminate_cnt = UNKNOWN;
    /* handle unequal start times, if any */
    if (t0 < input->t0) sound_prepend_zeros(input, t0);
    if (t0 < feedback->t0) sound_prepend_zeros(feedback, t0);
    /* minimum start time over all inputs: */
    t0_min = min(input->t0, min(feedback->t0, t0));
    /* how many samples to toss before t0: */
    susp->susp.toss_cnt = (long) ((t0 - t0_min) * sr + 0.5);
    if (susp->susp.toss_cnt > 0) {
	susp->susp.keep_fetch = susp->susp.fetch;
	susp->susp.fetch = alpasscv_toss_fetch;
    }

    /* initialize susp state */
    susp->susp.free = alpasscv_free;
    susp->susp.sr = sr;
    susp->susp.t0 = t0;
    susp->susp.mark = alpasscv_mark;
    susp->susp.print_tree = alpasscv_print_tree;
    susp->susp.name = "alpasscv";
    susp->susp.log_stop_cnt = UNKNOWN;
    susp->susp.current = 0;
    susp->input = input;
    susp->input_cnt = 0;
    susp->feedback = feedback;
    susp->feedback_cnt = 0;
    return sound_create((snd_susp_type)susp, t0, sr, scale_factor);
}


sound_type snd_alpasscv(sound_type input, time_type delay, sound_type feedback)
{
    sound_type input_copy = sound_copy(input);
    sound_type feedback_copy = sound_copy(feedback);
    return snd_make_alpasscv(input_copy, delay, feedback_copy);
}
