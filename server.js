const http = require('http');
const fs = require('fs');
const path = require('path');

const publicDir = path.join(__dirname, 'public');
const PORT = process.env.PORT || 3030;
const app = http.createServer((req, res) => {
    // Your server logic here
});
app.listen(PORT, () => {
    console.log(`Server running on port ${PORT}`);
});
const mimeTypes = {
  '.html': 'text/html',
  '.css': 'text/css',
  '.js': 'text/javascript',
  '.json': 'application/json',
  '.png': 'image/png',
  '.jpg': 'image/jpeg',
  '.jpeg': 'image/jpeg',
  '.gif': 'image/gif',
  '.svg': 'image/svg+xml',
  '.ico': 'image/x-icon',
  '.txt': 'text/plain',
  '.c': 'text/plain'
};

// ===== CORS HEADERS =====
function setCORS(res) {
  res.setHeader('Access-Control-Allow-Origin', '*');
  res.setHeader('Access-Control-Allow-Methods', 'GET, POST, OPTIONS');
  res.setHeader('Access-Control-Allow-Headers', 'Content-Type');
}

function sendResponse(res, statusCode, contentType, data) {
  res.writeHead(statusCode, { 'Content-Type': contentType });
  res.end(data);
}

function sendJSON(res, statusCode, obj) {
  setCORS(res);
  res.writeHead(statusCode, { 'Content-Type': 'application/json' });
  res.end(JSON.stringify(obj));
}

// ===== READ DATA FILES =====
function loadReports() {
  try {
    const data = fs.readFileSync('report.txt', 'utf8').trim();
    if (!data) return [];
    return data.split('\n').map((line, i) => {
      const [flood, rain, zone] = line.trim().split(/\s+/).map(Number);
      return { id: i + 1, flood, rain, zone };
    });
  } catch {
    return [];
  }
}

function loadZones() {
  try {
    const data = fs.readFileSync('zone.txt', 'utf8').trim();
    if (!data) return [];
    return data.split('\n').map(line => {
      const [zone, houses] = line.trim().split(/\s+/).map(Number);
      return { zone, houses };
    });
  } catch {
    return [{ zone: 1, houses: 0 }, { zone: 2, houses: 0 }, { zone: 3, houses: 0 }];
  }
}

function saveReports(reports) {
  const lines = reports.map(r => `${r.flood} ${r.rain} ${r.zone}`).join('\n');
  fs.writeFileSync('report.txt', lines + (lines ? '\n' : ''));
}

function saveZones(zones) {
  const lines = zones.map(z => `${z.zone} ${z.houses}`).join('\n');
  fs.writeFileSync('zone.txt', lines + '\n');
}

// ===== CALCULATE ANNOUNCEMENT (Same as C program) =====
function calculateAnnouncement() {
  const zones = loadZones();
  const reports = loadReports();
  const announcement = [];

  for (let zone = 1; zone <= 3; zone++) {
    const zoneData = zones.find(z => z.zone === zone) || { houses: 0 };
    const zoneReports = reports.filter(r => r.zone === zone);
    const totalReports = zoneReports.length;

    const floodCount = [0, 0, 0, 0];
    const rainCount = [0, 0, 0, 0];
    zoneReports.forEach(r => {
      if (r.flood >= 1 && r.flood <= 3) floodCount[r.flood]++;
      if (r.rain >= 1 && r.rain <= 3) rainCount[r.rain]++;
    });

    let highestFlood = 1;
    for (let i = 3; i >= 1; i--) {
      if (floodCount[i] > 0) { highestFlood = i; break; }
    }

    let domRain = 1;
    for (let i = 2; i <= 3; i++) {
      if (rainCount[i] > rainCount[domRain]) domRain = i;
    }

    const floodBase = highestFlood === 1 ? 0.20 : highestFlood === 2 ? 0.50 : 0.90;
    const rainMod = domRain === 1 ? -0.05 : domRain === 2 ? 0.00 : 0.10;
    let totalImpact = floodBase + rainMod;
    if (totalImpact < 0.10) totalImpact = 0.10;
    if (totalImpact > 1.00) totalImpact = 1.00;

    const estimatedAffected = Math.max(1, Math.floor(zoneData.houses * totalImpact));

    const ankleRatio = zoneData.houses > 0 ? floodCount[1] / zoneData.houses : 0;
    const kneeRatio = zoneData.houses > 0 ? floodCount[2] / zoneData.houses : 0;
    const waistRatio = zoneData.houses > 0 ? floodCount[3] / zoneData.houses : 0;

    let color = 0;
    if (ankleRatio >= 0.15 && color < 1) color = 1;
    if (floodCount[2] > 0) {
      if (kneeRatio >= 0.25) color = 2;
      else if (kneeRatio >= 0.10 && color < 1) color = 1;
    }
    if (floodCount[3] > 0) {
      if (waistRatio >= 0.15) color = 2;
      else if (waistRatio >= 0.05 && color < 1) color = 1;
    }

    const status = color === 0 ? 'GREEN' : color === 1 ? 'YELLOW' : 'RED';
    const roadStatus = color === 0 ? 'Passable' : color === 1 ? 'Passable with Caution' : 'NOT PASSABLE';
    const highestFloodStr = highestFlood === 1 ? 'Ankle' : highestFlood === 2 ? 'Knee' : 'Waist';
    const domRainStr = domRain === 1 ? 'Light' : domRain === 2 ? 'Moderate' : 'Heavy';

    announcement.push({
      zone,
      status,
      roadStatus,
      houses: zoneData.houses,
      totalReports,
      estimatedAffected,
      highestFlood: highestFloodStr,
      dominantRain: domRainStr,
      impact: totalImpact
    });
  }

  return announcement;
}

// ===== SERVE STATIC FILES =====
function serveFile(req, res) {
  let requestPath = req.url.split('?')[0].replace(/\/\//g, '/');
  if (requestPath === '/') {
    requestPath = '/index.html';
  }

  const safePath = path.normalize(path.join(publicDir, requestPath));
  if (!safePath.startsWith(publicDir)) {
    sendResponse(res, 403, 'text/plain', 'Access denied');
    return;
  }

  fs.stat(safePath, (err, stats) => {
    if (err) {
      sendResponse(res, 404, 'text/plain', 'File not found');
      return;
    }

    if (stats.isDirectory()) {
      serveFile({ url: path.join(requestPath, 'index.html') }, res);
      return;
    }

    const ext = path.extname(safePath).toLowerCase();
    const contentType = mimeTypes[ext] || 'application/octet-stream';

    fs.readFile(safePath, (readErr, data) => {
      if (readErr) {
        sendResponse(res, 500, 'text/plain', 'Server error');
        return;
      }

      setCORS(res);
      sendResponse(res, 200, contentType, data);
    });
  });
}

// ===== PARSE JSON BODY =====
function parseBody(req, callback) {
  let body = '';
  req.on('data', chunk => body += chunk);
  req.on('end', () => {
    try {
      callback(JSON.parse(body));
    } catch {
      callback(null);
    }
  });
}

// ===== MAIN SERVER =====
const server = http.createServer((req, res) => {
  const url = req.url.split('?')[0];

  // Handle CORS preflight
  if (req.method === 'OPTIONS') {
    setCORS(res);
    res.writeHead(204);
    res.end();
    return;
  }

  // ===== API ROUTES =====
  
  // GET /api/reports
  if (url === '/api/reports' && req.method === 'GET') {
    sendJSON(res, 200, { reports: loadReports() });
    return;
  }

  // GET /api/zones
  if (url === '/api/zones' && req.method === 'GET') {
    sendJSON(res, 200, { zones: loadZones() });
    return;
  }

  // GET /api/announcement
  if (url === '/api/announcement' && req.method === 'GET') {
    sendJSON(res, 200, { announcement: calculateAnnouncement() });
    return;
  }

  // POST /api/report
  if (url === '/api/report' && req.method === 'POST') {
    parseBody(req, (body) => {
      if (!body || !body.flood || !body.rain || !body.zone) {
        sendJSON(res, 400, { success: false, error: 'Invalid data' });
        return;
      }

      const { flood, rain, zone } = body;
      const zones = loadZones();
      const reports = loadReports();

      const zoneData = zones.find(z => z.zone === zone) || { houses: 0 };
      const zoneReportLimit = Math.max(zoneData.houses, 1);
      const zoneReports = reports.filter(r => r.zone === zone);

      let overwritten = false;

      // OVERWRITE LOGIC: If full, remove oldest
      if (zoneReports.length >= zoneReportLimit) {
        const oldestIndex = reports.findIndex(r => r.zone === zone);
        if (oldestIndex !== -1) {
          reports.splice(oldestIndex, 1);
          overwritten = true;
        }
      }

      reports.push({ id: reports.length + 1, flood, rain, zone });
      saveReports(reports);

      sendJSON(res, 200, { success: true, overwritten });
    });
    return;
  }

  // POST /api/zone
  if (url === '/api/zone' && req.method === 'POST') {
    parseBody(req, (body) => {
      if (!body || !body.username || !body.password) {
        sendJSON(res, 400, { success: false, error: 'Missing credentials' });
        return;
      }

      const { zone, houses, username, password } = body;

      if (username !== 'admin' || password !== '1234') {
        sendJSON(res, 403, { success: false, error: 'Access Denied' });
        return;
      }

      const zones = loadZones();
      const idx = zones.findIndex(z => z.zone === zone);
      if (idx !== -1) {
        zones[idx].houses = houses;
      } else {
        zones.push({ zone, houses });
      }

      saveZones(zones);
      sendJSON(res, 200, { success: true });
    });
    return;
  }

  // ===== STATIC FILES =====
  serveFile(req, res);
});

server.listen(port, () => {
  console.log(`🌐 Server running at http://localhost:${port}/`);
  console.log(`   API endpoints:`);
  console.log(`   - GET  /api/reports`);
  console.log(`   - GET  /api/zones`);
  console.log(`   - GET  /api/announcement`);
  console.log(`   - POST /api/report`);
  console.log(`   - POST /api/zone`);
});