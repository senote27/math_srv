# Variables
CC = gcc
CFLAGS = -Wall -Wextra -Werror
LDFLAGS = -lm

# Directories
SRCDIR = src
CLIENTDIR = client
OBJDIR = objects

# Server-related variables
SERVER_SRC = $(SRCDIR)/server.c
SERVER_OBJ = $(patsubst $(SRCDIR)/%.c,$(OBJDIR)/%.o,$(SERVER_SRC))
SERVER_EXE = server

# Client-related variables
CLIENT_SRC = $(CLIENTDIR)/client.c
CLIENT_OBJ = $(patsubst $(CLIENTDIR)/%.c,$(OBJDIR)/%.o,$(CLIENT_SRC))
CLIENT_EXE = client_program  # Change the executable name to client_program

# Matrix Inversion program-related variables
MATINV_SRC = $(SRCDIR)/matrix_inverse.c
MATINV_OBJ = $(patsubst $(SRCDIR)/%.c,$(OBJDIR)/%.o,$(MATINV_SRC))
MATINV_EXE = matinvpar

# K-Means program-related variables
KMEANS_SRC = $(SRCDIR)/kmeans.c
KMEANS_OBJ = $(patsubst $(SRCDIR)/%.c,$(OBJDIR)/%.o,$(KMEANS_SRC))
KMEANS_EXE = kmeanspar

# All-related variables
ALL_EXES = $(SERVER_EXE) $(CLIENT_EXE) $(MATINV_EXE) $(KMEANS_EXE)

# Targets
all: $(ALL_EXES)

$(SERVER_EXE): $(SERVER_OBJ)
	$(CC) $(CFLAGS) -o $@ $^ $(LDFLAGS)

$(CLIENT_EXE): $(CLIENT_OBJ)
	$(CC) $(CFLAGS) -o $@ $^ $(LDFLAGS)

$(MATINV_EXE): $(MATINV_OBJ)
	$(CC) $(CFLAGS) -o $@ $^ $(LDFLAGS)

$(KMEANS_EXE): $(KMEANS_OBJ)
	$(CC) $(CFLAGS) -o $@ $^ $(LDFLAGS)

$(OBJDIR)/%.o: $(SRCDIR)/%.c
	@mkdir -p $(OBJDIR)
	$(CC) $(CFLAGS) -c $< -o $@

$(OBJDIR)/%.o: $(CLIENTDIR)/%.c
	@mkdir -p $(OBJDIR)
	$(CC) $(CFLAGS) -c $< -o $@

clean:
	rm -rf $(OBJDIR)/* $(ALL_EXES)

.PHONY: all clean
