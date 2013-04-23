#include <stdarg.h>
#include "xrlterror.h"


#define XRLT_GET_VAR_STR(msg, str) {                                          \
    int       size;                                                           \
    int       chars;                                                          \
    char     *larger;                                                         \
    va_list   ap;                                                             \
                                                                              \
    str = (char *)xrltMalloc(150);                                            \
    if (str == NULL)                                                          \
        return;                                                               \
                                                                              \
    size = 150;                                                               \
                                                                              \
    while (1) {                                                               \
        va_start(ap, msg);                                                    \
        chars = vsnprintf(str, size, msg, ap);                                \
        va_end(ap);                                                           \
        if (chars > -1 && chars < size)                                       \
            break;                                                            \
        if (chars > -1)                                                       \
            size += chars + 1;                                                \
        else                                                                  \
            size += 100;                                                      \
        if ((larger = (char *)xrltRealloc(str, size)) == NULL) {              \
            xrltFree(str);                                                    \
            return;                                                           \
        }                                                                     \
        str = larger;                                                         \
    }                                                                         \
}


/**
 * xrltGenericErrorDefaultFunc:
 * @ctx:  an error context
 * @msg:  the message to display/transmit
 * @...:  extra parameters for the message
 *
 * Default handler for out of context error messages.
 */
static void
xrltGenericErrorDefaultFunc(void *ctx ATTRIBUTE_UNUSED, const char *msg, ...)
{
    va_list args;

    if (xrltGenericErrorContext == NULL) {
        xrltGenericErrorContext = (void *)stderr;
    }

    va_start(args, msg);
    vfprintf((FILE *)xrltGenericErrorContext, msg, args);
    va_end(args);
}


xmlGenericErrorFunc   xrltGenericError = xrltGenericErrorDefaultFunc;
void                 *xrltGenericErrorContext = NULL;


/**
 * xrltSetGenericErrorFunc:
 * @ctx:  the new error handling context
 * @handler:  the new handler function
 *
 * Function to reset the handler and the error context for out of
 * context error messages.
 * This simply means that @handler will be called for subsequent
 * error messages while not parsing nor validating. And @ctx will
 * be passed as first argument to @handler
 * One can simply force messages to be emitted to another FILE * than
 * stderr by setting @ctx to this file handle and @handler to NULL.
 */
void
xrltSetGenericErrorFunc(void *ctx, xmlGenericErrorFunc handler)
{
    xrltGenericErrorContext = ctx;

    xmlSetGenericErrorFunc(ctx, handler);

    if (handler != NULL) {
        xrltGenericError = handler;
    } else {
        xrltGenericError = xrltGenericErrorDefaultFunc;
    }
}


/**
 * xrltPrintErrorContext:
 * @ctxt:  the transformation context
 * @xrl:  the XRLT document
 * @node:  the current node being processed
 *
 * Display the context of an error.
 */
void
xrltPrintErrorContext(xrltContextPtr ctxt, xrltRequestsheetPtr sheet,
                      xmlNodePtr node)
{
    int                   line = 0;
    const xmlChar        *file = NULL;
    const xmlChar        *name = NULL;
    const xmlChar        *ns = NULL;
    const char           *type = "Error";
    xmlGenericErrorFunc   error = xrltGenericError;
    void                 *errctx = xrltGenericErrorContext;

    if (ctxt != NULL) {
        ctxt->error = TRUE;
    }

    if (node != NULL)  {
        if (node->type == XML_DOCUMENT_NODE ||
            node->type == XML_HTML_DOCUMENT_NODE)
        {
            xmlDocPtr doc = (xmlDocPtr)node;

            file = doc->URL;
        } else {
            line = xmlGetLineNo(node);
            if (node->doc != NULL && node->doc->URL != NULL) {
                file = node->doc->URL;
            }

            if (node->name != NULL) {
                name = node->name;

                if (node->ns != NULL && node->ns->prefix != NULL) {
                    ns = node->ns->prefix;
                }
            }
        }
    }

    if (ctxt != NULL) {
        type = "Runtime error";
    } else if (sheet != NULL) {
        type = "Compilation error";
    }

    if (file != NULL && line != 0 && name != NULL) {
        if (ns == NULL) {
            error(errctx, "%s: file '%s' line %d element '%s': ",
                  type, file, line, name);
        } else {
            error(errctx, "%s: file '%s' line %d element '%s:%s': ",
                  type, file, line, ns, name);
        }
    } else if (file != NULL && name != NULL) {
        if (ns == NULL) {
            error(errctx, "%s: file '%s' element '%s': ", type, file, name);
        } else {
            error(errctx,
                  "%s: file '%s' element '%s:%s': ", type, file, ns, name);
        }
    } else if (file != NULL && line != 0) {
        error(errctx, "%s: file '%s' line %d: ", type, file, line);
    } else if (file != NULL) {
        error(errctx, "%s: file '%s': ", type, file);
    } else if (name != NULL) {
        if (ns == NULL) {
            error(errctx, "%s: element '%s': ", type, name);
        } else {
            error(errctx, "%s: element '%s:%s': ", type, ns, name);
        }
    } else {
        error(errctx, "%s: ", type);
    }
}


/**
 * xrltTransformError:
 * @ctxt:  an XRLT transformation context
 * @style:  the XRLT stylesheet used
 * @node:  the current node in the stylesheet
 * @msg:  the message to display/transmit
 * @...:  extra parameters for the message display
 *
 * Display and format an error messages, gives file, line, position and
 * extra parameters, will use the specific transformation context if available
 */
void
xrltTransformError(xrltContextPtr ctxt, xrltRequestsheetPtr sheet,
                   xmlNodePtr node, const char *msg, ...)
{
    xmlGenericErrorFunc   error = xrltGenericError;
    void                 *errctx = xrltGenericErrorContext;
    char                 *str;

    if (ctxt != NULL) {
        ctxt->error = TRUE;
    }

    xrltPrintErrorContext(ctxt, sheet, node);

    XRLT_GET_VAR_STR(msg, str);

    error(errctx, "%s", str);

    if (str != NULL) { xrltFree(str); }
}
