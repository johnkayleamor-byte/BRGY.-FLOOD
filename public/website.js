const API_BASE = 'http://localhost:3030/api';

let zones = [];
let reports = [];
let announcement = [];

const FLOOD_NAMES = { 1: "Ankle", 2: "Knee", 3: "Waist" };
const RAIN_NAMES = { 1: "Light", 2: "Moderate", 3: "Heavy" };

async function fetchAPI(endpoint, options = {}) {
    try {
        const response = await fetch(API_BASE + endpoint, {
            ...options,
            headers: {
                'Content-Type': 'application/json',
                ...options.headers
            }
        });
        const data = await response.json();
        if (!response.ok) throw new Error(data.error || 'API Error');
        return data;
    } catch (error) {
        console.error('API Error:', error);
        throw error;
    }
}

async function loadZones() {
    try {
        const data = await fetchAPI('/zones');
        zones = data.zones || [];
    } catch (error) {
        console.error('Failed to load zones:', error);
        zones = [{ zone: 1, houses: 0 }, { zone: 2, houses: 0 }, { zone: 3, houses: 0 }];
    }
}

async function loadReports() {
    try {
        const data = await fetchAPI('/reports');
        reports = data.reports || [];
    } catch (error) {
        console.error('Failed to load reports:', error);
        reports = [];
    }
}

async function loadAnnouncement() {
    try {
        const data = await fetchAPI('/announcement');
        announcement = data.announcement || [];

        announcement = data.announcement || [];

    } catch (error) {
        console.error('Failed to load announcement:', error);
        announcement = [];
    }
}

function showSection(sectionId) {
    const currentSection = document.querySelector('.section.active');
    if (currentSection && currentSection.id === sectionId) return;

    document.querySelectorAll('.section').forEach(s => s.classList.remove('active'));
    document.querySelectorAll('.nav-tab').forEach(t => t.classList.remove('active'));

    document.getElementById(sectionId).classList.add('active');

    document.querySelectorAll('.nav-tab').forEach(tab => {
        if (tab.getAttribute('onclick')?.includes(`'${sectionId}'`)) {
            tab.classList.add('active');
        }
    });

    if (sectionId === 'dashboard') renderDashboard();
    if (sectionId === 'data') loadData();
}

function showAlert(containerId, type, message) {
    const container = document.getElementById(containerId);
    if (!container) return;
    const icons = { success: '✅', error: '❌', warning: '⚠️' };
    container.innerHTML = `<div class="alert alert-${type}">${icons[type]} ${message}</div>`;
    setTimeout(() => container.innerHTML = '', 4000);
}

async function checkZoneCapacity() {
    await loadZones();
    await loadReports();

    const zone = Number(document.getElementById('zone').value);
    const zoneData = zones.find(z => z.zone === zone);
    const zoneHouses = zoneData ? zoneData.houses : 0;
    const zoneReports = reports.filter(r => r.zone === zone).length;

    const warning = document.getElementById('overwriteWarning');
    if (!warning) return;

    if (zoneReports >= zoneHouses && zoneHouses > 0) {
        warning.style.display = 'block';
        warning.innerHTML = `⚠️ <strong>Zone ${zone} is FULL!</strong> (${zoneReports}/${zoneHouses} reports). New report will overwrite the oldest.`;
    } else {
        warning.style.display = 'none';
    }
}

async function submitReport() {
    const flood = Number(document.getElementById('flood').value);
    const rain = Number(document.getElementById('rain').value);
    const zone = Number(document.getElementById('zone').value);

    try {
        const data = await fetchAPI('/report', {
            method: 'POST',
            body: JSON.stringify({ flood, rain, zone })
        });

        const msg = data.overwritten
            ? `✅ Report saved! (Overwrote oldest report for Zone ${zone})`
            : `✅ Report saved for Zone ${zone}!`;

        showAlert('reportAlert', 'success', msg);
        clearReportForm();
        
        setTimeout(() => {
            showSection('dashboard');
        }, 2000);

    } catch (error) {
        showAlert('reportAlert', 'error', error.message || 'Failed to save report');
    }
}

function clearReportForm() {
    document.getElementById('flood').value = '1';
    document.getElementById('rain').value = '1';
    document.getElementById('zone').value = '1';
    const warning = document.getElementById('overwriteWarning');
    if (warning) warning.style.display = 'none';
}

async function updateZone() {
    const username = document.getElementById('username').value;
    const password = document.getElementById('password').value;
    const zone = Number(document.getElementById('update-zone').value);
    const houses = Number(document.getElementById('houses').value);

    if (!username || !password) {
        showAlert('adminAlert', 'error', 'Please enter username and password');
        return;
    }

    if (isNaN(houses) || houses < 0) {
        showAlert('adminAlert', 'error', 'Invalid number of houses!');
        return;
    }

    try {
        await fetchAPI('/zone', {
            method: 'POST',
            body: JSON.stringify({ zone, houses, username, password })
        });

        showAlert('adminAlert', 'success', `Purok ${zone} updated! Houses: ${houses}`);
        await loadZones();

    } catch (error) {
        showAlert('adminAlert', 'error', error.message || 'Failed to update zone');
    }
}

async function renderDashboard() {
    await loadZones();
    await loadReports();
    await loadAnnouncement();

    document.getElementById('totalReports').textContent = reports.length;
    document.getElementById('totalHouses').textContent = zones.reduce((sum, z) => sum + (z.houses || 0), 0);
    document.getElementById('totalPuroks').textContent = zones.length;

    const container = document.getElementById('zoneCards');
    if (!container) return;
    container.innerHTML = '';

    let activeAlerts = 0;

    if (!announcement || announcement.length === 0) {
        container.innerHTML = '<div style="padding: 20px; color: #8892b0; text-align: center;">No zone data available.</div>';
        document.getElementById('activeAlerts').textContent = 0;
        return;
    }

    announcement.forEach(zoneData => {
        if (!zoneData) return;

        const status = zoneData.status || 'GREEN';
        if (status !== 'GREEN') activeAlerts++;

        const statusColors = { GREEN: 'green', YELLOW: 'yellow', RED: 'red' };
        const statusText = status + ' ZONE';
        
        // Check if there are reports for this zone
        const hasReports = zoneData.totalReports > 0;

        const card = document.createElement('div');
        card.className = `zone-card ${statusColors[status] || 'green'}`;
        card.innerHTML = `
            <div class="zone-card-header">
                <div class="zone-number">Purok ${zoneData.zone || '?'}</div>
                <div class="zone-status-badge">${statusText}</div>
            </div>
            <div class="zone-stats">
                <div class="zone-stat">
                    <div class="zone-stat-value">${zoneData.houses || 0}</div>
                    <div class="zone-stat-label">Residents</div>
                </div>
                <div class="zone-stat">
                    <div class="zone-stat-value">${zoneData.totalReports || 0}</div>
                    <div class="zone-stat-label">Reports</div>
                </div>
                <div class="zone-stat">
                    <div class="zone-stat-value">${hasReports ? (zoneData.estimatedAffected || 0) : '—'}</div>
                    <div class="zone-stat-label">Est. Affected</div>
                </div>
            </div>
            <div class="zone-advisory">
                <strong>Road:</strong> ${zoneData.roadStatus || 'Passable'}<br>
                <strong>Highest Flood:</strong> ${hasReports ? (zoneData.highestFlood || 'None') : 'No data'}<br>
                <strong>Dominant Rain:</strong> ${hasReports ? (zoneData.dominantRain || 'None') : 'No data'}
            </div>
            ${!hasReports ? '<div class="zone-no-data">No reports submitted yet</div>' : ''}
        `;
        container.appendChild(card);
    });

    document.getElementById('activeAlerts').textContent = activeAlerts;
}

async function loadData() {
    await loadZones();
    await loadReports();
    await loadAnnouncement();

    // Zone table
    const zoneBody = document.getElementById('zoneTableBody');
    if (zoneBody) {
        zoneBody.innerHTML = '';

        zones.forEach(z => {
            const zoneData = announcement.find(a => a.zone === z.zone);
            const status = zoneData ? zoneData.status : 'GREEN';
            const totalReports = zoneData ? zoneData.totalReports : 0;
            const statusColors = { GREEN: '#4caf50', YELLOW: '#ff9800', RED: '#f44336' };

            zoneBody.innerHTML += `
                <tr>
                    <td>Purok ${z.zone}</td>
                    <td>${z.houses || 0}</td>
                    <td>${totalReports}</td>
                    <td><span style="color: ${statusColors[status] || '#4caf50'}; font-weight: bold;">${status}</span></td>
                </tr>
            `;
        });
    }

    // Reports table
    const reportBody = document.getElementById('reportTableBody');
    if (reportBody) {
        reportBody.innerHTML = '';
        reports.forEach((r, i) => {
            reportBody.innerHTML += `
                <tr>
                    <td>${i + 1}</td>
                    <td><span class="badge badge-${FLOOD_NAMES[r.flood]?.toLowerCase() || 'ankle'}">${FLOOD_NAMES[r.flood] || 'Unknown'}</span></td>
                    <td><span class="badge badge-${RAIN_NAMES[r.rain]?.toLowerCase() || 'light'}">${RAIN_NAMES[r.rain] || 'Unknown'}</span></td>
                    <td>Purok ${r.zone}</td>
                </tr>
            `;
        });
    }
}

function updateClock() {
    const el = document.getElementById('currentTime');
    if (el) el.textContent = new Date().toLocaleString();
}

async function initApp() {
    // ✅ Load data first, then render
    await loadZones();
    await loadReports();
    await loadAnnouncement();
    
    renderDashboard();
    updateClock();
    setInterval(updateClock, 1000);
}

window.addEventListener('load', initApp);