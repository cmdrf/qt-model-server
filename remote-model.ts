/* remote-model.ts

BSD 2-Clause License

Copyright (c) 2018-2020, Fabian Herb
All rights reserved.

Redistribution and use in source and binary forms, with or without
modification, are permitted provided that the following conditions are met:

* Redistributions of source code must retain the above copyright notice, this
  list of conditions and the following disclaimer.

* Redistributions in binary form must reproduce the above copyright notice,
  this list of conditions and the following disclaimer in the documentation
  and/or other materials provided with the distribution.

THIS SOFTWARE IS PROVIDED BY THE COPYRIGHT HOLDERS AND CONTRIBUTORS "AS IS"
AND ANY EXPRESS OR IMPLIED WARRANTIES, INCLUDING, BUT NOT LIMITED TO, THE
IMPLIED WARRANTIES OF MERCHANTABILITY AND FITNESS FOR A PARTICULAR PURPOSE ARE
DISCLAIMED. IN NO EVENT SHALL THE COPYRIGHT HOLDER OR CONTRIBUTORS BE LIABLE
FOR ANY DIRECT, INDIRECT, INCIDENTAL, SPECIAL, EXEMPLARY, OR CONSEQUENTIAL
DAMAGES (INCLUDING, BUT NOT LIMITED TO, PROCUREMENT OF SUBSTITUTE GOODS OR
SERVICES; LOSS OF USE, DATA, OR PROFITS; OR BUSINESS INTERRUPTION) HOWEVER
CAUSED AND ON ANY THEORY OF LIABILITY, WHETHER IN CONTRACT, STRICT LIABILITY,
OR TORT (INCLUDING NEGLIGENCE OR OTHERWISE) ARISING IN ANY WAY OUT OF THE USE
OF THIS SOFTWARE, EVEN IF ADVISED OF THE POSSIBILITY OF SUCH DAMAGE.
*/

import { BehaviorSubject } from 'rxjs';

export class RemoteModel {
  private items: any;
  private keyItem: string = "id";
  private socket: WebSocket;
  private subject: BehaviorSubject<any[]> = new BehaviorSubject([]);
  
  constructor(private url: string) {
    this.socket = new WebSocket(this.url);
    this.socket.onmessage = ((msg) => {
      var obj = JSON.parse(msg.data);

      if(obj.operation == "data") {
        this.items = obj.items;
      }
      if(obj.operation == "rowData") {
        this.items = obj.items;
        this.keyItem = obj.key;
      }
      else if(obj.operation == "inserted") {
        for(var id in obj.items) {
          var item = obj.items[id];
          this.items[id] = item;
        }
      }
      else if(obj.operation == "rowsInserted") {
        this.items.splice(obj.start, 0, ...obj.items);
      }
      else if(obj.operation == "removed") {
        for(var id in obj.items) {
          delete this.items[id];
        }
      }
      else if(obj.operation == "rowsRemoved") {
        this.items.splice(obj.start, obj.end - obj.start + 1);
      }
      else if(obj.operation == "dataChanged") {
        for(var id in obj.items) {
          var item = obj.items[id];
          if(this.items.hasOwnProperty(id))
            this.items[id] = item;
        }
      }
      else if(obj.operation == "rowDataChanged") {
        this.items.splice(obj.start, obj.end - obj.start + 1, ...obj.items);

      }
      if(Array.isArray(this.items))
        this.subject.next(this.items);
      else
        this.subject.next(Object.keys(this.items).map(key => this.items[key]));       
    });
    this.socket.onerror = this.subject.error.bind(this.subject);
    this.socket.onclose = this.subject.complete.bind(this.subject);
  }

  getItems(): BehaviorSubject<any[]> {
    return this.subject;
  }

  editItem(item: any) {
    let id: string = String(item[this.keyItem]);
    let items = {};
    items[id] = item;
    let msg = {
      operation: "changeData",
      items: items
    };
    this.socket.send(JSON.stringify(msg));
  }

  removeItem(id: string) {
    let msg = {
      operation: "remove",
      items: [id]
    };
    this.socket.send(JSON.stringify(msg));
  }

  insertItem(item: any) {
    let items;
    if(item.hasOwnProperty(this.keyItem)) {
      items = {};
      items[String(item[this.keyItem])] = item;
    }
    else {
      items = [item];
    }
    let msg = {
      operation: "insert",
      items: items
    };
    this.socket.send(JSON.stringify(msg));
  }
}
