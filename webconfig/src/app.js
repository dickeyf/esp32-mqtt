import {HttpClient} from 'aurelia-http-client';
import {inject} from 'aurelia-framework';

@inject(HttpClient)
export class App {
  message = 'Hello World!';
  
  constructor(httpClient) {
    this.httpClient = httpClient;
  }
  
  onScan() {  
    this.httpClient.get('/wifi/networks')
      .then(response => {
        console.log(response.content);
        this.wifi_list = response.content.wifi_list;
        console.log(this.wifi_list);
      });
  }
  
  onConnect() {  
    this.httpClient.post('/settings', {
      wifi_ssid: this.wifi_ssid,
      wifi_password: this.wifi_password,
      mqtt_url: this.mqtt_url,
      mqtt_username: this.mqtt_username,
      mqtt_password: this.mqtt_password
    })
      .then(response => {
        console.log(response.content);
        this.wifi_list = response.content.wifi_list;
        console.log(this.wifi_list);
      });
  }
  
  onTest() {
    this.httpClient.get('/settings')
    .then(response => {
      console.log(response.content);
    });
  }
}
