/******************************************************************************\
 Plutocracy GenDoc - Copyright (C) 2008 - Michael Levin

 This program is free software; you can redistribute it and/or modify it under
 the terms of the GNU General Public License as published by the Free Software
 Foundation; either version 2, or (at your option) any later version.

 This program is distributed in the hope that it will be useful, but WITHOUT
 ANY WARRANTY; without even the implied warranty of MERCHANTABILITY or FITNESS
 FOR A PARTICULAR PURPOSE. See the GNU General Public License for more details.
\******************************************************************************/

#include "gendoc.h"

/******************************************************************************\
 Searches for an entry.
\******************************************************************************/
entry_t *D_entry_find(entry_t *entry, const char *name)
{
        while (entry) {
                int cmp;

                cmp = strcmp(name, entry->name);
                if (cmp < 0)
                        break;
                if (!cmp)
                        return entry;
                entry = entry->next;
        }
        return NULL;
}

/******************************************************************************\
 Updates an entry if it exists or creates a new one.
\******************************************************************************/
void D_entry_add(const entry_t *current, entry_t **root)
{
        entry_t *entry, *prev, *cur;
        int cmp;

        prev = NULL;
        cur = *root;
        while (cur) {
                cmp = strcmp(current->name, cur->name);
                if (cmp < 0)
                        break;
                else if (!cmp) {
                        *cur = *current;
                        return;
                }
                prev = cur;
                cur = cur->next;
        }
        if (!prev) {
                entry = calloc(sizeof (*entry), 1);
                *entry = *current;
                entry->next = *root;
                *root = entry;
                return;
        }
        entry = calloc(sizeof (*entry), 1);
        *entry = *current;
        entry->next = prev->next;
        prev->next = entry;
}

/******************************************************************************\
 Prints an entry linked list.
\******************************************************************************/
void D_output_entries(FILE *file, entry_t *entry)
{
        while (entry) {
                fprintf(file, "<p><b>%s</b>\n<pre>%s</pre>\n%s</p>\n",
                        entry->name, entry->def, entry->comment);
                entry = entry->next;
        }
}

