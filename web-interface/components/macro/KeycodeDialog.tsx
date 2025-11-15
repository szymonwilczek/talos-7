'use client';

import React, { useState } from 'react';
import {
  Dialog,
  DialogContent,
  DialogDescription,
  DialogHeader,
  DialogTitle,
  DialogTrigger,
} from '@/components/ui/dialog';
import { Button } from '@/components/ui/button';
import { Input } from '@/components/ui/input';
import { ScrollArea } from '@/components/ui/scroll-area';
import {
  Table,
  TableBody,
  TableCell,
  TableHead,
  TableHeader,
  TableRow,
} from '@/components/ui/table';
import { Tabs, TabsContent, TabsList, TabsTrigger } from '@/components/ui/tabs';
import { HID_KEYCODES, KEYCODE_CATEGORIES } from '@/lib/data/hid-keycodes';

export function KeycodeDialog() {
  const [search, setSearch] = useState('');

  const filteredKeycodes = search
    ? HID_KEYCODES.filter(
      (k) =>
        k.key.toLowerCase().includes(search.toLowerCase()) ||
        k.code.toString().includes(search)
    )
    : HID_KEYCODES;

  return (
    <Dialog>
      <DialogTrigger asChild>
        <Button variant="outline">HID Keycode Reference</Button>
      </DialogTrigger>
      <DialogContent className="max-w-3xl max-h-[80vh]">
        <DialogHeader>
          <DialogTitle>HID Keycode Reference</DialogTitle>
          <DialogDescription>
            Reference table for USB HID keyboard keycodes
          </DialogDescription>
        </DialogHeader>

        <div className="space-y-4">
          <Input
            placeholder="Search by key name or code..."
            value={search}
            onChange={(e) => setSearch(e.target.value)}
          />

          {search ? (
            <ScrollArea className="h-[500px]">
              <Table>
                <TableHeader>
                  <TableRow>
                    <TableHead>Key</TableHead>
                    <TableHead>Code</TableHead>
                    <TableHead>Category</TableHead>
                  </TableRow>
                </TableHeader>
                <TableBody>
                  {filteredKeycodes.map((keycode) => (
                    <TableRow key={keycode.code}>
                      <TableCell className="font-medium">{keycode.key}</TableCell>
                      <TableCell>{keycode.code}</TableCell>
                      <TableCell>{keycode.category}</TableCell>
                    </TableRow>
                  ))}
                </TableBody>
              </Table>
            </ScrollArea>
          ) : (
            <Tabs defaultValue={KEYCODE_CATEGORIES[0]}>
              <TabsList>
                {KEYCODE_CATEGORIES.map((cat) => (
                  <TabsTrigger key={cat} value={cat}>
                    {cat}
                  </TabsTrigger>
                ))}
              </TabsList>

              {KEYCODE_CATEGORIES.map((category) => (
                <TabsContent key={category} value={category}>
                  <ScrollArea className="h-[400px]">
                    <Table>
                      <TableHeader>
                        <TableRow>
                          <TableHead>Key</TableHead>
                          <TableHead>Code</TableHead>
                        </TableRow>
                      </TableHeader>
                      <TableBody>
                        {HID_KEYCODES.filter((k) => k.category === category).map(
                          (keycode) => (
                            <TableRow key={keycode.code}>
                              <TableCell className="font-medium">
                                {keycode.key}
                              </TableCell>
                              <TableCell>{keycode.code}</TableCell>
                            </TableRow>
                          )
                        )}
                      </TableBody>
                    </Table>
                  </ScrollArea>
                </TabsContent>
              ))}
            </Tabs>
          )}
        </div>
      </DialogContent>
    </Dialog>
  );
}
