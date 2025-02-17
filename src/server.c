#include "server.h"

#include "jansson.h"
#include "map.h"
#include "parser.h"
#include "sema.h"

static json_t *mx_range_to_json(MXRange range) {
    json_t *range_obj = json_object();
    json_t *start = json_object();
    json_object_set_new(start, "line", json_integer(range.start.row));
    json_object_set_new(start, "character", json_integer(range.start.col));
    json_object_set_new(range_obj, "start", start);
    json_t *end = json_object();
    json_object_set_new(end, "line", json_integer(range.end.row));
    json_object_set_new(end, "character", json_integer(range.end.col));
    json_object_set_new(range_obj, "end", end);
    return range_obj;
}

static void send(json_t *response) {
    char *response_str = json_dumps(response, JSON_COMPACT);
    printf("Content-Length: %zu\r\n\r\n%s\r\n", strlen(response_str),
           response_str);
    fprintf(stderr, "Content-Length: %zu\n\n%s\n", strlen(response_str),
            response_str);
    fflush(stdout);
    free(response_str);
}

static json_t *mk_publish_diagnostics(const char *uri,
                                      MXDiagnosticList *diags) {
    Arena scratch = {0};
    arena_init(&scratch, ARENA_DEFAULT_RESERVE_SIZE);
    json_t *response = json_object();
    json_object_set_new(response, "jsonrpc", json_string("2.0"));
    json_object_set_new(response, "method",
                        json_string("textDocument/publishDiagnostics"));

    json_t *params = json_object();
    json_t *diagnostics = json_array();
    for (size_t i = 0; i < diags->size; i++) {
        MXDiagnostic *diag = arraylist_get(diags, i);
        json_t *diagnostic = json_object();
        json_object_set_new(diagnostic, "range", mx_range_to_json(diag->range));
        const char *message =
            mx_diagnostic_kind_to_string(&scratch, diag->kind);
        json_object_set_new(diagnostic, "message", json_string(message));
        json_object_set_new(diagnostic, "severity", json_integer(1));
        json_array_append_new(diagnostics, diagnostic);
    }
    json_object_set_new(params, "diagnostics", diagnostics);
    json_object_set_new(params, "uri", json_string(uri));
    json_object_set_new(response, "params", params);

    arena_release(&scratch);

    return response;
}

static json_t *mk_capabilities() {
    json_t *capabilities = json_object();
    json_t *textDocumentSync = json_object();
    json_object_set_new(textDocumentSync, "openClose", json_true());
    json_object_set_new(textDocumentSync, "change", json_integer(1));
    json_object_set_new(textDocumentSync, "willSave", json_false());
    json_object_set_new(textDocumentSync, "willSaveWaitUntil", json_false());
    json_t *save = json_object();
    json_object_set_new(save, "includeText", json_true());
    json_object_set_new(textDocumentSync, "save", save);
    json_object_set_new(capabilities, "textDocumentSync", textDocumentSync);
    return capabilities;
}

static json_t *mk_initialize(uint32_t id) {
    json_t *response = json_object();
    json_object_set_new(response, "jsonrpc", json_string("2.0"));
    json_object_set_new(response, "id", json_integer(id));

    // Build result object
    json_t *result = json_object();
    json_t *capabilities = mk_capabilities();

    // Attach capabilities to result
    json_object_set_new(result, "capabilities", capabilities);
    json_object_set_new(response, "result", result);

    return response;
}

static void initialize_handler(MXLangServer *server, json_t *request) {
    assert(server != NULL);
    assert(request != NULL);

    // Extract the ID
    json_t *id = json_object_get(request, "id");
    if (!id || !json_is_integer(id)) {
        fprintf(stderr, "No ID in request\n");
        return;
    }
    uint32_t id_value = json_integer_value(id);

    // Send the response
    json_t *response = mk_initialize(id_value);

    send(response);

    json_decref(response);
}

static void shutdown_handler(MXLangServer *server, json_t *request) {
    assert(server != NULL);
    assert(request != NULL);
    exit(0);
}

static void did_open_handler(MXLangServer *server, json_t *request) {
    assert(server != NULL);
    assert(request != NULL);
    // Extract the params
    json_t *params = json_object_get(request, "params");
    if (!params || !json_is_object(params)) {
        fprintf(stderr, "Invalid params in request\n");
        return;
    }

    // Extract the text document
    json_t *textDocument = json_object_get(params, "textDocument");
    if (!textDocument || !json_is_object(textDocument)) {
        fprintf(stderr, "Invalid textDocument in request\n");
        return;
    }

    // Extract the document URI
    json_t *uri = json_object_get(textDocument, "uri");
    if (!uri || !json_is_string(uri)) {
        fprintf(stderr, "Invalid URI in request\n");
        return;
    }
    const char *uri_value = json_string_value(uri);

    // Extract the document text
    json_t *text = json_object_get(textDocument, "text");
    if (!text || !json_is_string(text)) {
        fprintf(stderr, "Invalid text in request\n");
        return;
    }
    const char *text_value = json_string_value(text);

    // Store the document in the server
    hashmap_set(&server->arena, server->documents, uri_value,
                (void *)text_value);

    // Parse the document
    Ast *ast = parse(server->permanent_arena, text_value);
    Mxir *mxir = analyze(server->permanent_arena, ast);

    // Send diagnostics
    json_t *response = mk_publish_diagnostics(uri_value, &ast->diagnostics);

    send(response);
}

static void did_change_handler(MXLangServer *server, json_t *request) {
    assert(server != NULL);
    assert(request != NULL);
}

static void did_close_handler(MXLangServer *server, json_t *request) {
    assert(server != NULL);
    assert(request != NULL);

    // Extract the params
    json_t *params = json_object_get(request, "params");
    if (!params || !json_is_object(params)) {
        fprintf(stderr, "Invalid params in request\n");
        return;
    }

    // Extract the text document
    json_t *textDocument = json_object_get(params, "textDocument");
    if (!textDocument || !json_is_object(textDocument)) {
        fprintf(stderr, "Invalid textDocument in request\n");
        return;
    }

    // Extract the document URI
    json_t *uri = json_object_get(textDocument, "uri");
    if (!uri || !json_is_string(uri)) {
        fprintf(stderr, "Invalid URI in request\n");
        return;
    }
    const char *uri_value = json_string_value(uri);

    // Remove the document from the server
    hashmap_delete(server->documents, uri_value);
}

static void did_save_handler(MXLangServer *server, json_t *request) {
    assert(server != NULL);
    assert(request != NULL);

    // Extract the params
    json_t *params = json_object_get(request, "params");
    if (!params || !json_is_object(params)) {
        fprintf(stderr, "Invalid params in request\n");
        return;
    }

    // Extract the text document
    json_t *textDocument = json_object_get(params, "textDocument");
    if (!textDocument || !json_is_object(textDocument)) {
        fprintf(stderr, "Invalid textDocument in request\n");
        return;
    }

    // Extract the document URI
    json_t *uri = json_object_get(textDocument, "uri");
    if (!uri || !json_is_string(uri)) {
        fprintf(stderr, "Invalid URI in request\n");
        return;
    }
    const char *uri_value = json_string_value(uri);

    // Extract the document text
    json_t *text = json_object_get(params, "text");
    if (!text || !json_is_string(text)) {
        fprintf(stderr, "Invalid text in request\n");
        return;
    }
    const char *text_value = json_string_value(text);

    // Store the document in the server
    hashmap_set(&server->arena, server->documents, uri_value,
                (void *)text_value);

    // Parse the document
    Ast *ast = parse(server->permanent_arena, text_value);
    Mxir *mxir = analyze(server->permanent_arena, ast);

    // Send diagnostics
    json_t *response = mk_publish_diagnostics(uri_value, &ast->diagnostics);

    send(response);
}

// Function type for a request handler
typedef void (*RequestHandler)(MXLangServer *server, json_t *request);

// Lookup table for request handlers
static struct {
    const char *method;
    RequestHandler handler;
} handlers[] = {{"initialize", initialize_handler},
                {"shutdown", shutdown_handler},
                {"textDocument/didOpen", did_open_handler},
                {"textDocument/didClose", did_close_handler},
                {"textDocument/didChange", did_change_handler},
                {"textDocument/didSave", did_save_handler}};

#define BUFFER_SIZE 8192

char *read_message() {
    char buffer[BUFFER_SIZE];
    size_t content_length = 0;

    // Read headers
    while (fgets(buffer, BUFFER_SIZE, stdin)) {
        // Detect empty line (end of headers)
        if (strcmp(buffer, "\r\n") == 0 || strcmp(buffer, "\n") == 0) {
            break;
        }

        // Parse Content-Length header
        if (strncmp(buffer, "Content-Length:", 15) == 0) {
            content_length = atoi(buffer + 15);
        }
    }

    // Validate content length
    if (content_length <= 0) {
        fprintf(stderr, "Invalid Content-Length\n");
        return NULL;
    }

    // Allocate memory for the JSON payload
    char *json_payload = (char *)malloc(content_length + 1);
    if (!json_payload) {
        fprintf(stderr, "Memory allocation failed\n");
        return NULL;
    }

    // Read the JSON-RPC request body
    size_t read_bytes = fread(json_payload, 1, content_length, stdin);
    if (read_bytes != content_length) {
        fprintf(stderr, "Failed to read full JSON-RPC payload\n");
        free(json_payload);
        return NULL;
    }

    json_payload[content_length] = '\0'; // Null-terminate the string

    return json_payload;
}

void mx_lang_server_init(Arena *permanent_arena, MXLangServer *server) {
    server->permanent_arena = permanent_arena;
    arena_init(&server->arena, ARENA_DEFAULT_RESERVE_SIZE);
    server->documents = hashmap_init(&server->arena);
}

void mx_lang_server_listen(MXLangServer *server) {
    fprintf(stderr, "Listening for LSP messages\n");

    server->running = true;

    while (server->running) {
        fprintf(stderr, "\nWaiting for message\n");
        const char *message_text = read_message();

        json_error_t error;
        json_t *message = json_loads(message_text, 0, &error);
        if (!message || !json_is_object(message)) {
            fprintf(stderr, "Error parsing JSON: %s\n", error.text);
            continue;
        }

        // Extract the method
        const char *method =
            json_string_value(json_object_get(message, "method"));
        if (!method) {
            fprintf(stderr, "No method in request\n");
            continue;
        }

        // Find the handler for the method
        RequestHandler handler = NULL;
        for (size_t i = 0; i < sizeof(handlers) / sizeof(handlers[0]); i++) {
            if (strcmp(handlers[i].method, method) == 0) {
                handler = handlers[i].handler;
                break;
            }
        }

        fprintf(stderr, "Handler found: %p\n", (void *)handler);

        // Call the handler
        if (handler) {
            fprintf(stderr, "Handling method: %s\n", method);
            handler(server, message);
        } else {
            fprintf(stderr, "No handler for method: %s\n", method);
        }
    }
}

void server(Arena *permanent_arena) {
    MXLangServer server = {0};
    mx_lang_server_init(permanent_arena, &server);
    mx_lang_server_listen(&server);
}
