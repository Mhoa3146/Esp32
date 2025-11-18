const API_URL = 'http://localhost:3000/api/light-logs';

document.addEventListener('DOMContentLoaded', () => {

    fetchLogs();

    const btn = document.getElementById('btnRefresh');
    btn.addEventListener('click', fetchLogs);

    setInterval(fetchLogs, 3000);
});

async function fetchLogs() {
    const tbody = document.getElementById('logTableBody');
    const statusSpan = document.getElementById('lastUpdate');
    
    try {
        const response = await fetch(API_URL);
        
        if (!response.ok) {
            throw new Error('Network response was not ok');
        }

        const logs = await response.json();

        tbody.innerHTML = '';

        if (logs.length === 0) {
            tbody.innerHTML = '<tr><td colspan="2" style="text-align:center">ไม่มีประวัติการใช้งาน</td></tr>';
            return;
        }

        logs.forEach(log => {
            const row = document.createElement('tr');
            
            const isPn = log.status === 'ON';
            const statusClass = isPn ? 'status-on' : 'status-off';
            const statusText = isPn ? ' เปิดไฟ (ON)' : ' ปิดไฟ (OFF)';

            row.innerHTML = `
                <td><span class="status-badge ${statusClass}">${statusText}</span></td>
                <td>${log.timestamp}</td>
            `;
            
            tbody.appendChild(row);
        });

        const now = new Date();
        statusSpan.innerText = `อัปเดตล่าสุด: ${now.toLocaleTimeString('th-TH')}`;

    } catch (error) {
        console.error('Error fetching logs:', error);
        statusSpan.innerText = "Error: เชื่อมต่อ Server ไม่ได้";
        statusSpan.style.color = "red";
    }
}