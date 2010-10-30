/*
 * "$Id: image.h,v 1.2 2005/10/18 02:08:16 rlk Exp $"
 *
 *   libgimpprint image functions.
 *
 *   Copyright 1997-2000 Michael Sweet (mike@easysw.com) and
 *	Robert Krawitz (rlk@alum.mit.edu)
 *
 *   This program is free software; you can redistribute it and/or modify it
 *   under the terms of the GNU General Public License as published by the Free
 *   Software Foundation; either version 2 of the License, or (at your option)
 *   any later version.
 *
 *   This program is distributed in the hope that it will be useful, but
 *   WITHOUT ANY WARRANTY; without even the implied warranty of MERCHANTABILITY
 *   or FITNESS FOR A PARTICULAR PURPOSE.  See the GNU General Public License
 *   for more details.
 *
 *   You should have received a copy of the GNU General Public License
 *   along with this program; if not, write to the Free Software
 *   Foundation, Inc., 59 Temple Place - Suite 330, Boston, MA 02111-1307, USA.
 */

/**
 * @file gutenprint/image.h
 * @brief Image functions.
 */

#ifndef GUTENPRINT_IMAGE_H
#define GUTENPRINT_IMAGE_H

#ifdef __cplusplus
extern "C" {
#endif

/**
 * The image type is an abstract data type for interfacing with the
 * image creation program.
 *
 * @defgroup image image
 * @{
 */

/*
 * Constants...
 */

/*! The maximum number of channels. */
#define STP_CHANNEL_LIMIT	(32)


/** Image status. */
typedef enum
{
  STP_IMAGE_STATUS_OK,   /*!< Everything is OK. */
  STP_IMAGE_STATUS_ABORT /*!< An error occured, or the job was aborted. */
} stp_image_status_t;

/**
 * The image type is an abstract data type for interfacing with the
 * image creation program.  It provides callbacks to functions defined
 * within the client application which are called while printing the
 * image.
 */
typedef struct stp_image
{
  /**
   * This callback is used to perform any initialization required by
   * the image layer for the image.  It will be called once per image.
   * @param image the image in use.
   */
  void (*init)(struct stp_image *image);
  /**
   * This callback is called to reset the image to the beginning.  It
   * may (in principle) be called multiple times if a page is being
   * printed more than once.
   * @warning The reset() call may be removed in the future.
   * @param image the image in use.
   */
  void (*reset)(struct stp_image *image);
  /**
   * This callback returns the width of the image in pixels.
   * @param image the image in use.
   */
  int  (*width)(struct stp_image *image);
  /**
   * This callback returns the height of the image in pixels.
   * @param image the image in use.
   */
  int  (*height)(struct stp_image *image);
  /**
   * This callback transfers the data from the image to the gimp-print
   * library.  It is called from the driver layer.  It should copy
   * WIDTH (as returned by the width() member) pixels of data into the
   * data buffer.  It normally returns STP_IMAGE_STATUS_OK; if
   * something goes wrong, or the application wishes to stop producing
   * any further output (e. g. because the user cancelled the print
   * job), it should return STP_IMAGE_STATUS_ABORT.  This will cause
   * the driver to flush any remaining data to the output.  It will
   * always request rows in monotonically ascending order, but it may
   * skip rows (if, for example, the resolution of the input is higher
   * than the resolution of the output).
   * @param image the image in use.
   * @param data a pointer to width() bytes of pixel data.
   * @param byte_limit (image width * number of channels).
   * @param row (unused).
   */
  stp_image_status_t (*get_row)(struct stp_image *image, unsigned char *data,
                                size_t byte_limit, int row);

  /**
   * This callback returns the name of the application.  This is
   * embedded in the output by some drivers.
   */
  const char *(*get_appname)(struct stp_image *image);
  /**
   * This callback is called at the end of each page.
   */
  void (*conclude)(struct stp_image *image);
  /**
   * A pointer to an application-specific state information that might
   * need to be associated with the image object.
   */
  void *rep;
} stp_image_t;

extern void stp_image_init(stp_image_t *image);
extern void stp_image_reset(stp_image_t *image);
extern int stp_image_width(stp_image_t *image);
extern int stp_image_height(stp_image_t *image);
extern stp_image_status_t stp_image_get_row(stp_image_t *image,
					    unsigned char *data,
					    size_t limit, int row);
extern const char *stp_image_get_appname(stp_image_t *image);
extern void stp_image_conclude(stp_image_t *image);

  /** @} */

#ifdef __cplusplus
  }
#endif

#endif /* GUTENPRINT_IMAGE_H */
/*
 * End of "$Id: image.h,v 1.2 2005/10/18 02:08:16 rlk Exp $".
 */
